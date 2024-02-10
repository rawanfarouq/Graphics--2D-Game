#include <ctime> // Include this for time functions
#include <cstdlib>
#include <string>
#include <sstream>
#include <glut.h>
#include <iostream>
#include <SDL.h>
#include <SDL_mixer.h>



// Define global game state variables
int playerX, playerY;      // Player's initial X and Y position
const int maxHealth = 5; // Maximum health points
int playerHealth;
int playerScore;           // Player's initial score
int gameTime;              // Initial game time (in seconds)
time_t lastTime; // To store the last time when the game loop was executed
float playerAngle;         // Player's rotation angle
float playerSpeed;         // Player's speed
bool isGameOver;
bool isGameWon;
int goalX, goalY;

int windowWidth = 700;
int windowHeight = 450;
int gameWidth = 300;
int gameHeight = 300;

// Global variables for animation
float collectableRotation = 0.0f;
float collectableScale = 1.0f;
bool collectableGrowing = true;

Mix_Chunk* collisionSound;
Mix_Chunk* collectSound;
Mix_Chunk* winSound;
Mix_Chunk* loseSound;




const int maxObstacles = 5;    // Maximum number of obstacles
const int maxCollectables = 7; // Maximum number of collectables
const int maxPowerUps = 2;     // Maximum number of power-ups

// Define game objects (obstacles, collectables, power-ups) with positions
struct GameObject {
    float x;
    float y;
    bool active;
};

GameObject obstacles[maxObstacles];
GameObject collectables[maxCollectables];



// Define power-up types
enum PowerUpType {
    POWER_UP_1,
    POWER_UP_2
};

struct PowerUp {
    float x;
    float y;
    bool active;
    time_t activationTime; // Store activation time
    PowerUpType type;
    int duration; // Duration of the power-up effect in seconds
    bool working;

};


PowerUp powerUp1, powerUp2;
PowerUp powerUps[maxPowerUps];


// Define a structure for the goal
struct Goal {
    float x;
    float y;
    bool active;
};

Goal goal;



// Function to handle power-up collision
void HandlePowerUpCollision(PowerUp& powerUp) {
    float dx = powerUp.x - playerX;
    float dy = powerUp.y - playerY;
    float distance = sqrt(dx * dx + dy * dy);

    if (distance < 10 && powerUp.active) {
        // Apply the power-up effect based on the type
        if (powerUp.type == POWER_UP_1) {
            playerSpeed += 2.0f; // Increase player's speed
            Mix_PlayChannel(-1, collectSound, 0);

        }
        else if (powerUp.type == POWER_UP_2) {
            playerScore += 20; // Increase player's lives
            Mix_PlayChannel(-1, collectSound, 0);

        }

        powerUp.active = false; // Power-up disappears when collected
        powerUp.working = true;
    }
}


// Function to update power-up status
void UpdatePowerUps() {
    //time_t currentTime = time(nullptr);   //remove this 
    for (int i = 0; i < maxPowerUps; i++) {
        if (powerUps[i].active) {
            // Handle power-up collision
            HandlePowerUpCollision(powerUps[i]);

        }
    }

}



// Function to handle player collision boundaries
void HandleCollisions() {
    if (playerX < 0 || playerX > gameWidth || playerY < 0 || playerY > gameHeight) {
        playerHealth--; // Decrease player's lives upon collision with boundaries
        Mix_PlayChannel(-1, collisionSound, 0);


    }
}


// Callback function to resume the background sound after a delay
Uint32 ResumeBackgroundSound(Uint32 interval, void* param) {
    // Resume the background sound
    Mix_Resume(-1);
    return 0;  // Timer only runs once
}

// Function to check for collisions between the player and the goal
void HandleGoalCollision() {
    if (goal.active) {
        float dx = goal.x - playerX;
        float dy = goal.y - playerY;
        float distance = sqrt(dx * dx + dy * dy);

        // Define a collision radius (adjust this as needed)
        float collisionRadius = 23.0f;

        if (distance < collisionRadius) {
            // Pause the background sound
            Mix_Pause(-1);

            // Handle goal collision (player has reached the goal)
            isGameWon = true;
            Mix_PlayChannel(-1, winSound, 0);

            // Resume the background sound after a delay (adjust the delay as needed)
            SDL_AddTimer(9000, ResumeBackgroundSound, NULL);
        }
    }
}



void UpdateHealthBar() {
    glPushMatrix();
    glLoadIdentity();

    // Draw the word "Health" above the health bar
    glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white
    glRasterPos2f(10.0f, 286.0f); // Adjust the position as needed
    const char* text = "Health";
    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text[i]);
    }

    // Set the position, size, and color for the health bar
    glTranslatef(30.0f, 285.0f, 0.0f); // Top-left corner
    glScalef(playerHealth, 1.0f, 1.0f); // Adjust the width based on player's health
    glColor3f(0.0f, 1.0f, 0.0f); // Green color for health bar

    // Draw the health bar as a shorter rectangle
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(20.0f, 0.0f); // Adjust the width as needed
    glVertex2f(20.0f, 10.0f); // Adjust the height as needed
    glVertex2f(0.0f, 10.0f);
    glEnd();

    glPopMatrix();
}



// Function to display a game over message
void DisplayGameOverMessage() {
    glRasterPos2f(100.0f, 150.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    std::string gameOverText = "Game Over!";
    for (char c : gameOverText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}


// Function to check if a position is too close to existing objects
bool IsPositionOccupied(float x, float y) {
    float minDistance = 20.0f; // Adjust this value as needed

    for (int i = 0; i < maxObstacles; i++) {
        if (obstacles[i].active) {
            float dx = obstacles[i].x - x;
            float dy = obstacles[i].y - y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < minDistance) {
                return true; // Position is too close to an obstacle
            }
        }
    }

    for (int i = 0; i < maxCollectables; i++) {
        if (collectables[i].active) {
            float dx = collectables[i].x - x;
            float dy = collectables[i].y - y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < minDistance) {
                return true; // Position is too close to a collectable
            }
        }
    }

    for (int i = 0; i < maxPowerUps; i++) {
        if (powerUps[i].active) {
            float dx = powerUps[i].x - x;
            float dy = powerUps[i].y - y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < minDistance) {
                return true; // Position is too close to a power-up
            }
        }
    }

    // No collision detected, position is not occupied
    return false;
}


void InitializeGame() {

    // Set player's initial position in the middle of the screen
    playerX = gameWidth / 2;
    playerY = gameHeight / 2;

    // Reset player's lives and score
    playerScore = 0;

    // Set initial game time (you may need to adjust this)
    gameTime = 15;

    // Initialize the goal's position
    goal.x = 150; // Adjust the X position as needed
    goal.y = 25; // Adjust the Y position as needed
    goal.active = true;

    playerAngle = 0.0f; // Set the initial angle to 0 degrees (facing up)

    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));

    // Initialize playerSpeed to a non-zero value (e.g., 2.0f)
    playerSpeed = 2.0f;

    // Initialize obstacles
    obstacles[0].x = 40.0f;
    obstacles[0].y = 100.0f;
    obstacles[0].active = true;

    obstacles[1].x = 40.0f;
    obstacles[1].y = 250.0f;
    obstacles[1].active = true;

    obstacles[2].x = 140.0f;
    obstacles[2].y = 98.0f;
    obstacles[2].active = true;

    obstacles[3].x = 220.0f;
    obstacles[3].y = 265.0f;
    obstacles[3].active = true;

    obstacles[4].x = 220.0f;
    obstacles[4].y = 165.0f;
    obstacles[4].active = true;


    for (int i = 0; i < maxCollectables; i++) {
        // Generate random positions, checking for collisions with the player, goal, and obstacles
        do {
            collectables[i].x = rand() % (gameWidth - 40) + 20;
            collectables[i].y = rand() % (gameHeight - 40) + 20;
        } while (IsPositionOccupied(collectables[i].x, collectables[i].y) ||
            (abs(collectables[i].x - playerX) < 20 && abs(collectables[i].y - playerY) < 20) ||
            (abs(collectables[i].x - goal.x) < 40 && abs(collectables[i].y - goal.y) < 40));

        // Check for collisions with obstacles
        for (int j = 0; j < maxObstacles; j++) {
            if (abs(collectables[i].x - obstacles[j].x) < 20 && abs(collectables[i].y - obstacles[j].y) < 20) {
                // There's a collision with an obstacle, so regenerate the collectable's position
                do {
                    collectables[i].x = rand() % (gameWidth - 40) + 20;
                    collectables[i].y = rand() % (gameHeight - 40) + 20;
                } while (IsPositionOccupied(collectables[i].x, collectables[i].y) ||
                    (abs(collectables[i].x - playerX) < 30 && abs(collectables[i].y - playerY) < 30) ||
                    (abs(collectables[i].x - goal.x) < 40 && abs(collectables[i].y - goal.y) < 40));
                // Restart the loop to check for collisions again
                j = -1;
            }
        }

        collectables[i].active = true;
    }

    for (int i = 0; i < maxPowerUps; i++) {
        // Generate random positions, checking for collisions with the player, goal, and obstacles
        do {
            powerUps[i].x = rand() % (gameWidth - 40) + 20;
            powerUps[i].y = rand() % (gameHeight - 40) + 20;
        } while (IsPositionOccupied(powerUps[i].x, powerUps[i].y) ||
            (abs(powerUps[i].x - playerX) < 30 && abs(powerUps[i].y - playerY) < 30) ||
            (abs(powerUps[i].x - goal.x) < 40 && abs(powerUps[i].y - goal.y) < 40));

        // Check for collisions with obstacles
        for (int j = 0; j < maxObstacles; j++) {
            if (abs(powerUps[i].x - obstacles[j].x) < 20 && abs(powerUps[i].y - obstacles[j].y) < 20) {
                // There's a collision with an obstacle, so regenerate the power-up's position
                do {
                    powerUps[i].x = rand() % (gameWidth - 40) + 20;
                    powerUps[i].y = rand() % (gameHeight - 40) + 20;
                } while (IsPositionOccupied(powerUps[i].x, powerUps[i].y) ||
                    (abs(powerUps[i].x - playerX) < 30 && abs(powerUps[i].y - playerY) < 30) ||
                    (abs(powerUps[i].x - goal.x) < 40 && abs(powerUps[i].y - goal.y) < 40));
                // Restart the loop to check for collisions again
                j = -1;
            }
        }

        powerUps[i].active = true;
        powerUps[i].type = static_cast<PowerUpType>(i); // You can choose the type as needed
        powerUps[i].working = false;

        if (powerUps[i].type == POWER_UP_1) {
            powerUps[i].duration = 8; // Default duration for POWER_UP_1
        }
        else if (powerUps[i].type == POWER_UP_2) {
            powerUps[i].duration = 8; // Default duration for POWER_UP_2
        }
    }


    isGameOver = false;
    isGameWon = false;


}

void CheckGameWin() {
    // Check the game win condition (e.g., reaching a specific score)
    if (playerScore >= 50) {
        Mix_Pause(-1);

        // Handle goal collision (player has reached the goal)
        isGameWon = true;
        Mix_PlayChannel(-1, winSound, 0);

        // Resume the background sound after a delay (adjust the delay as needed)
        SDL_AddTimer(8000, ResumeBackgroundSound, NULL);
    }
}

// Function to handle time updates
void UpdateTime() {
    time_t currentTime = time(nullptr);
    double elapsedTime = difftime(currentTime, lastTime);

    if (!isGameOver && !isGameWon) {
        gameTime -= static_cast<int>(elapsedTime); // Decrement game time

        // Check for game win
        CheckGameWin();



        if (gameTime <= 0) {
            Mix_Pause(-1);

            // Handle goal collision (player has reached the goal)
            isGameOver = true;

            Mix_PlayChannel(-1, loseSound, 0);

            // Resume the background sound after a delay (adjust the delay as needed)
            SDL_AddTimer(8000, ResumeBackgroundSound, NULL);
        }
    }

    lastTime = currentTime;
}

// Modify the UpdatePlayerLives function to update player health
void UpdatePlayerHealth() {
    HandleGoalCollision();

    if (playerHealth <= 0) {
        Mix_Pause(-1);

        // Handle goal collision (player has reached the goal)
        isGameOver = true;
        Mix_PlayChannel(-1, loseSound, 0);

        // Resume the background sound after a delay (adjust the delay as needed)
        SDL_AddTimer(8000, ResumeBackgroundSound, NULL);
    }
}


// Function to handle key presses
void HandleKeypress(unsigned char key, int x, int y) {

    int newX = playerX;
    int newY = playerY;
    int halfPlayerSize = 10; // Half of the player's size

    switch (key) {
    case 'w': // Move up
        newY += playerSpeed;
        playerAngle = 0.0f; // Set the player's angle to face up
        break;
    case 's': // Move down
        newY -= playerSpeed;
        playerAngle = 180.0f; // Set the player's angle to face down
        break;
    case 'd': // Move right
        newX += playerSpeed;
        playerAngle = 270.0f; // Set the player's angle to face left
        break;
    case 'a': // Move left
        newX -= playerSpeed;
        playerAngle = 90.0f; // Set the player's angle to face right
        break;
    }

    // Check if the new position is within the boundaries
    if (newX - halfPlayerSize >= 0 && newX + halfPlayerSize <= 300 &&
        newY - halfPlayerSize >= 0 && newY + halfPlayerSize <= 300) {
        // Check collisions with obstacles
        bool canMove = true; // Variable to check if the player can move

        for (int i = 0; i < maxObstacles; i++) {
            if (obstacles[i].active) {
                float dx = obstacles[i].x - newX;
                float dy = obstacles[i].y - newY;
                float distance = sqrt(dx * dx + dy * dy);
                if (distance < halfPlayerSize + 10) {
                    // Decrease player's health if touching an obstacle
                    playerHealth--;
                    playerX = gameWidth / 2;
                    playerY = gameHeight / 2;
                    // Play the collision sound effect
                    Mix_PlayChannel(-1, collisionSound, 0);
                    canMove = false;
                    break; // Exit the loop if there's a collision
                }
            }
        }

        if (canMove) {
            // Update the player's position if no collision with obstacles
            playerX = newX;
            playerY = newY;
        }
    }
    else {
        // Player attempted to move partially out of bounds, reduce health
        playerHealth--;
    }

    // Handle collectable collisions and score increase
    for (int i = 0; i < maxCollectables; i++) {
        if (collectables[i].active) {
            float dx = collectables[i].x - playerX;
            float dy = collectables[i].y - playerY;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 10) {
                collectables[i].active = false;
                playerScore += 10; // Increase the score upon collecting
                Mix_PlayChannel(-1, collectSound, 0);

            }
        }
    }

    // Update player lives and health bar
   // UpdatePlayerLives();
    UpdatePlayerHealth();



    glutPostRedisplay(); // Redraw the scene


}



// Add a timer function that calls UpdateTime
void Timer(int value) {
    UpdateTime(); // Call the UpdateTime function

    for (int i = 0; i < maxPowerUps; i++) {
        if (powerUps[i].working) {
            powerUps[i].duration--;
            // Check if the power-up duration has expired
            if (powerUps[i].duration <= 0) {
                if (powerUps[i].type == POWER_UP_1) {
                    playerSpeed -= 2.0f; // Reverse the speed increase
                }
                else if (powerUps[i].type == POWER_UP_2) {
                    playerScore -= 20; // Decrease lives if it was a life power-up
                }
                powerUps[i].working = false; // lets reset it back to false when duration = 0 
            }
        }
        glutPostRedisplay(); // Redraw the scene
        glutTimerFunc(1000, Timer, 0); // Set the timer for 1 second (1000 milliseconds)
    }
}

void DrawPlayerPrimitives() {
    // Save the current transformation matrix
    glPushMatrix();

    // Translate to player's position
    glTranslatef(playerX, playerY, 0.0f);

    // Rotate based on the player's angle
    glRotatef(playerAngle, 0.0f, 0.0f, 1.0f);

    // Draw the player using different primitive shapes

    // Draw an outer circle
    int numSegments = 100;
    float outerRadius = 10.0f;  // Smaller outer circle
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.0f, 0.0f, 1.0f);  // Blue color for the outer circle
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= numSegments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(numSegments);
        float x = outerRadius * cosf(theta);
        float y = outerRadius * sinf(theta);
        glVertex2f(x, y);
    }
    glEnd();

    // Draw a pentagon
    float pentagonSize = 9.0f;  // Size of the pentagon
    glBegin(GL_POLYGON);
    glColor3f(1.0f, 1.0f, 0.0f);  // Yellow color for the pentagon
    for (int i = 0; i < 5; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / 5.0f;
        float x = pentagonSize * cosf(theta);
        float y = pentagonSize * sinf(theta);
        glVertex2f(x, y);
    }
    glEnd();

    // Draw a square
    float squareSize = 6.0f;  // Smaller square
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);  // Black color for the square
    glVertex2f(-squareSize, squareSize);
    glVertex2f(squareSize, squareSize);
    glVertex2f(squareSize, -squareSize);
    glVertex2f(-squareSize, -squareSize);
    glEnd();

    // Draw a triangle
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);  // Red color for the triangle
    glVertex2f(0.0f, 10.0f);  // Adjusted position
    glVertex2f(-5.0f, -5.0f);  // Adjusted position
    glVertex2f(5.0f, -5.0f);  // Adjusted position
    glEnd();

    // Restore the previous transformation matrix
    glPopMatrix();
}



void DrawCollectables() {
    for (int i = 0; i < maxCollectables; i++) {
        if (collectables[i].active) {
            glPushMatrix();
            glTranslatef(collectables[i].x, collectables[i].y, 0.0f);

            // Apply animation to the collectables (rotation and scale)
            glRotatef(collectableRotation, 0.0f, 0.0f, 1.0f);
            glScalef(collectableScale, collectableScale, 1.0f);

            // Define the number of concentric shapes
            int numShapes = 3;
            float rotation = 360.0f / numShapes;

            for (int j = 0; j < numShapes; j++) {
                if (j == 0) glColor3f(1.0f, 0.0f, 1.0f); // Red
                else if (j == 1) glColor3f(0.0f, 1.0f, 1.0f); // Green
                else if (j == 2)glColor3f(0.0f, 0.0f, 0.0f); // Blue

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(0.0f, 0.0f);

                for (int k = 0; k <= 360; k += 120) {
                    float radius = 12.0f - j * 5.0f;
                    float angle = (k + j * rotation) * 3.14159265358979323846f / 180.0f;
                    float x = radius * cosf(angle);
                    float y = radius * sinf(angle);
                    glVertex2f(x, y);
                }
                glEnd();

                glDisable(GL_BLEND);
            }

            glPopMatrix();
        }

    }

    // Animate the collectables
    // Reset the rotation if it exceeds 360 degrees
    collectableRotation += 6.0f;
    if (collectableRotation >= 360.0f) {
        collectableRotation = 0.0f;
    }

    // Scale the collectables
    if (collectableGrowing) {
        collectableScale += 0.01f;
    }
    else {
        collectableScale -= 0.01f;
    }

    // Reverse the scaling direction when the scale limit is reached
    if (collectableScale > 1.2f || collectableScale < 0.8f) {
        collectableGrowing = !collectableGrowing;
    }
}



// Function to draw power-ups
void DrawPowerUps() {
    for (int i = 0; i < maxPowerUps; i++) {
        if (powerUps[i].active) {
            glPushMatrix();
            glTranslatef(powerUps[i].x, powerUps[i].y, 0.0f);

            if (powerUps[i].type == POWER_UP_1) {
                int numSegments = 60;
                float outerRadius = 8.0f + 2.0f * sin(2.0f * glutGet(GLUT_ELAPSED_TIME) * 0.001f); // Pulsation effect
                float innerRadius = 4.0f;

                for (int shape = 0; shape < 4; shape++) {
                    float scale = 1.0f - 0.2f * shape; // Scale factor for each shape

                    // Set the color for each shape
                    if (shape == 0) glColor3f(1.0f, 1.0f, 0.2f); // Cyan 1st shape
                    else if (shape == 1) glColor3f(0.0f, 0.47f, 0.73f); // Aegean Blue 2nd shape
                    else if (shape == 2) glColor3f(1.0f, 0.7f, 0.0f); // Honey (Yellowish-Orange) 3rd shape
                    else if (shape == 3) glColor3f(1.0f, 0.25f, 0.0f); // Persimmon Orange 4th shape

                    glBegin(GL_TRIANGLE_STRIP);
                    for (int j = 0; j <= numSegments; j++) {
                        float theta = 2.0f * 3.14159265358979323846f * float(j) / float(numSegments);
                        float xOuter = (outerRadius * scale) * cosf(theta);
                        float yOuter = (outerRadius * scale) * sinf(theta);
                        float xInner = (innerRadius * scale) * cosf(theta);
                        float yInner = (innerRadius * scale) * sinf(theta);
                        glVertex2f(xOuter, yOuter);
                        glVertex2f(xInner, yInner);
                    }
                    glEnd();
                }
            }

            else if (powerUps[i].type == POWER_UP_2) {
                glLineWidth(3.0f); // Set line width to make it bold
                float pulsationScale = 1.0f + 0.5f * sin(2.0f * glutGet(GLUT_ELAPSED_TIME) * 0.001f); // Pulsation effect

                for (int polygon = 0; polygon < 3; polygon++) {
                    // Set the color for each polygon
                    if (polygon == 0) glColor3f(1.0f, 0.14f, 0.14f); // Scarlet for the outermost polygon
                    else if (polygon == 1) glColor3f(1.0f, 0.56f, 0.0f); // Tangerine for the second polygon
                    else if (polygon == 2) glColor3f(0.0f, 0.0f, 0.55f); // Dark blue for the third polygon

                    glBegin(GL_POLYGON);
                    for (int j = 0; j < 6; j++) { // You can change the number of sides as needed
                        float angle = 2.0f * 3.14159265358979323846f * float(j) / 6;
                        float x = (8.0f - polygon * 2.0f) * pulsationScale * cosf(angle); // Decrease size for inner polygons
                        float y = (8.0f - polygon * 2.0f) * pulsationScale * sinf(angle);
                        glVertex3f(x, y, 0.0f);
                    }
                    glEnd();

                    // Draw the "X" shape inside the polygon
                    glColor3f(0.53f, 0.81f, 0.98f); // Light blue color for the "X"
                    glBegin(GL_LINES);
                    glVertex3f(-8.0f * pulsationScale, 0.0f, 0.0f);
                    glVertex3f(8.0f * pulsationScale, 0.0f, 0.0f);
                    glVertex3f(0.0f, -8.0f * pulsationScale, 0.0f);
                    glVertex3f(0.0f, 8.0f * pulsationScale, 0.0f);
                    glEnd();
                }

                glLineWidth(1.0f); // Reset line width to default
            }

            glPopMatrix();
        }
    }
}


// Function to draw the goal
void DrawGoal() {
    static float yOffset = 0.0f;
    static float xOffset = 0.0f;
    static float sizeFactor = 1.0f;

    yOffset = 0.2f * sin(2 * 3.14159265358979323846f * 0.1f * glutGet(GLUT_ELAPSED_TIME) / 1000.0f);
    xOffset = 0.1f * cos(2 * 3.14159265358979323846f * 0.05f * glutGet(GLUT_ELAPSED_TIME) / 1000.0f);
    sizeFactor = 0.9f + 0.1f * sin(2 * 3.14159265358979323846f * 0.2f * glutGet(GLUT_ELAPSED_TIME) / 1000.0f);

    if (goal.active) {
        glPushMatrix();
        glTranslatef(goal.x + xOffset, goal.y + yOffset, 0.0f);
        glScalef(sizeFactor, sizeFactor, 1.0f);

        // Draw horizontal lines to represent the net
        glColor3f(0.85f, 0.85f, 0.85f); // Gray color for goal net
        glLineWidth(2.0f);

        glBegin(GL_LINES);
        for (float y = -10.0f; y <= 10.0f; y += 2.0f) {
            glVertex3f(-15.0f, y, 0.0f);
            glVertex3f(15.0f, y, 0.0f);
        }
        glEnd();

        // Draw vertical lines to represent the net
        glBegin(GL_LINES);
        for (float x = -15.0f; x <= 15.0f; x += 2.0f) {
            glVertex3f(x, -10.0f, 0.0f);
            glVertex3f(x, 10.0f, 0.0f);
        }
        glEnd();

        // Draw rectangles to represent goal posts
        glColor3f(0.0f, 0.0f, 0.5f); // Gray color for goal posts
        glBegin(GL_QUADS);
        glVertex3f(13.0f, -0.5f, 0.0f);
        glVertex3f(15.0f, -0.5f, 0.0f);
        glVertex3f(15.0f, 0.5f, 0.0f);
        glVertex3f(13.0f, 0.5f, 0.0f);
        glEnd();
        glBegin(GL_QUADS);
        glVertex3f(-13.0f, -0.5f, 0.0f);
        glVertex3f(-15.0f, -0.5f, 0.0f);
        glVertex3f(-15.0f, 0.5f, 0.0f);
        glVertex3f(-13.0f, 0.5f, 0.0f);
        glEnd();

        // Draw a circle to represent the goal area
        glColor3f(0.0f, 0.3f, 0.0f); // Green color for the goal area
        int numSegments = 30;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0.0f, 0.0f);
        for (int i = 0; i <= numSegments; i++) {
            float theta = 2.0f * 3.14159265358979323846f * float(i) / float(numSegments);
            float x = 4.0f * cosf(theta);
            float y = 4.0f * sinf(theta);
            glVertex2f(x, y);
        }
        glEnd();

        glPopMatrix();
    }
}



void DrawGameBorder() {
    glLineWidth(3.0f); // Increase line width for the original outer border
    glBegin(GL_LINE_LOOP);

    // Define the colors for each vertex of the original outer border
    glColor3f(1.0f, 0.0f, 0.0f); // Red
    glVertex2f(5, 5); // Top-left corner

    glColor3f(0.0f, 1.0f, 0.0f); // Green
    glVertex2f(gameWidth + 5, 5); // Top-right corner

    glColor3f(0.0f, 0.0f, 1.0f); // Blue
    glVertex2f(gameWidth + 5, gameHeight + 5); // Bottom-right corner

    glColor3f(1.0f, 1.0f, 0.0f); // Yellow
    glVertex2f(5, gameHeight + 5); // Bottom-left corner
    glEnd();

    glLineWidth(3.0f); // Increase line width for the white inner border
    glBegin(GL_LINE_LOOP);

    // Define the colors for each vertex of the white inner border
    glColor3f(1.0f, 1.0f, 1.0f); // White
    glVertex2f(7, 7); // Top-left corner (slightly different)

    // Adjust the other vertices accordingly to match the game border
    glVertex2f(gameWidth + 3, 7); // Top-right corner
    glVertex2f(gameWidth + 3, gameHeight + 3); // Bottom-right corner
    glVertex2f(7, gameHeight + 3); // Bottom-left corner

    glEnd();
    glLineWidth(1.0f); // Restore the default line width for the original border
}


const int numStars = 100;
float starX[numStars];
float starY[numStars];

void InitializeStars() {
    for (int i = 0; i < numStars; i++) {
        starX[i] = static_cast<float>(rand() % windowWidth);
        starY[i] = static_cast<float>(rand() % windowHeight);
    }
}

// Function to update the starfield animation
void UpdateStars() {
    for (int i = 0; i < numStars; i++) {
        starY[i] -= 4.5f; // Move stars upwards
        if (starY[i] < 0.0f) {
            starY[i] = static_cast<float>(windowHeight);
            starX[i] = static_cast<float>(rand() % windowWidth);
        }
    }
}

// Function to draw the animated starfield background
void DrawStarfield() {
    glColor3f(1.0f, 1.0f, 1.0f); // Star color

    glBegin(GL_POINTS);
    for (int i = 0; i < numStars; i++) {
        glVertex2f(starX[i], starY[i]);
    }
    glEnd();
}



void Display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the animated starfield as the background
    UpdateStars();
    DrawStarfield();

    // Draw the game border
    DrawGameBorder();


    if (isGameWon) {
        // Display game win message when isGameWon is true
        glRasterPos2f(100.0f, 150.0f);
        glColor3f(0.0f, 1.0f, 0.0f); // Green color for win message
        std::string winText = "Congratulations! You Win!";
        for (char c : winText) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }
    else if (isGameOver) {
        // Display game over message when isGameOver is true
        DisplayGameOverMessage();
    }

    if (!isGameOver) {
        DrawPlayerPrimitives();
        DrawGameBorder(); // Draw scene boundaries
        DrawCollectables();
        UpdatePowerUps();
        DrawPowerUps(); // Draw power-ups
        DrawGoal();

        // Rotate and draw the player
        glPushMatrix(); // Save the current transformation matrix
        glTranslatef(playerX, playerY, 0.0f); // Translate to player's position
        glRotatef(playerAngle, 0.0f, 0.0f, 1.0f); // Rotate based on the player's angle
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 10.0f, 0.0f);
        glVertex3f(-10.0f, -10.0f, 0.0f);
        glVertex3f(10.0f, -10.0f, 0.0f);
        glEnd();
        glPopMatrix(); // Restore the previous transformation matrix

        // Draw obstacles
        for (int i = 0; i < maxObstacles; i++) {
            if (obstacles[i].active) {
                glPushMatrix();
                glTranslatef(obstacles[i].x, obstacles[i].y, 0.0f);

                // Draw the cloud using ellipses
                glColor3f(0.0f, 1.0f, 1.0f);  // Blue color for the cloud
                float cloudRadius = 8.0f;
                float cloudSeparation = 10.0f;
                int numEllipses = 3;  // You can adjust this for the cloud's complexity

                for (int j = 0; j < numEllipses; j++) {
                    float xOffset = cloudSeparation * (j - numEllipses / 2);
                    float ellipseWidth = cloudRadius * (1.0f - j * 0.1f);
                    float ellipseHeight = cloudRadius * 0.7f;
                    glBegin(GL_TRIANGLE_FAN);
                    for (int k = 0; k <= 360; k++) {
                        float radian = k * 3.14159265358979323846f / 180.0f;
                        float x = xOffset + ellipseWidth * cosf(radian);
                        float y = ellipseHeight * sinf(radian);
                        glVertex2f(x, y);
                    }
                    glEnd();
                }

                // Draw a circle as a second shape
                glColor3f(1.0f, 0.2f, 0.2f);  // Red color for the circle
                float circleRadius = 5.0f;
                glBegin(GL_TRIANGLE_FAN);
                for (int k = 0; k <= 360; k++) {
                    float radian = k * 3.14159265358979323846f / 180.0f;
                    float x = 0.0f + circleRadius * cosf(radian);
                    float y = 0.0f + circleRadius * sinf(radian);
                    glVertex2f(x, y);
                }
                glEnd();

                glPopMatrix();
            }
        }


        // Display player's health bar
        UpdateHealthBar();

        // Update power-up status
        UpdatePowerUps();

        // Display player's score
        glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white
        glRasterPos2f(150.0f, gameHeight - 10.0f); // Adjust the vertical position
        std::stringstream scoreText;
        scoreText << "Score: " << playerScore;
        for (char c : scoreText.str()) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c); // Use GLUT_BITMAP_HELVETICA_18 for larger text
        }

        // Display game time
        glRasterPos2f(250.0f, gameHeight - 10.0f); // Adjust the vertical position
        std::stringstream timeText;
        timeText << "Time: " << gameTime << "s";
        for (char c : timeText.str()) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c); // Use GLUT_BITMAP_HELVETICA_18 for larger text
        }

    }
    else {
        // Display game over message when isGameOver is true
        DisplayGameOverMessage();
    }

    glFlush();
}



int main(int argc, char** argv) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1; // Exit the program with an error code
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer initialization failed: %s\n", Mix_GetError());
        return 1; // Exit the program with an error code
    }

    // Load the background sound effect
    Mix_Chunk* backgroundSound = Mix_LoadWAV("C:\\Users\\user\\Downloads\\background.wav");
    if (backgroundSound == NULL) {
        printf("Failed to load background sound effect: %s\n", Mix_GetError());
        return 1; // Exit the program with an error code
    }

    // Load the collision sound effect
    collisionSound = Mix_LoadWAV("C:\\Users\\user\\Downloads\\collision.wav");
    if (collisionSound == NULL) {
        printf("Failed to load collision sound effect: %s\n", Mix_GetError());
        return 1; // Exit the program with an error code
    }

    // Load the collision sound effect
    collectSound = Mix_LoadWAV("C:\\Users\\user\\Downloads\\collect.wav");
    if (collectSound == NULL) {
        printf("Failed to load collect sound effect: %s\n", Mix_GetError());
        return 1; // Exit the program with an error code
    }

    winSound = Mix_LoadWAV("C:\\Users\\user\\Downloads\\win.wav");
    if (winSound == NULL) {
        printf("Failed to load win sound effect: %s\n", Mix_GetError());
        return 1; // Exit the program with an error code
    }

    loseSound = Mix_LoadWAV("C:\\Users\\user\\Downloads\\lose.wav");
    if (loseSound == NULL) {
        printf("Failed to load lose sound effect: %s\n", Mix_GetError());
        return 1; // Exit the program with an error code
    }


    // Play the background sound effect
    Mix_PlayChannel(0, backgroundSound, -1);

    glutInit(&argc, argv);

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(150, 150);

    // Create the window and set the title
    glutCreateWindow("2D Game");


    // Initialize lastTime
    lastTime = time(nullptr);

    // Set the keyboard callback function
    glutKeyboardFunc(HandleKeypress);

    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background color

    // Adjust the orthographic projection to fit the game area
    glViewport(0, 0, gameWidth + 10, gameHeight + 10);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, gameWidth + 10, 0.0, gameHeight + 10);
    glMatrixMode(GL_MODELVIEW);

    // Initialize the game
    InitializeGame();

    // Start the timer to update game time
    glutTimerFunc(1000, Timer, 0); // Start the timer for 1 second (1000 milliseconds)

    playerHealth = maxHealth;

    InitializeStars(); // Initialize the starfield

    glutDisplayFunc(Display);
    // glutIdleFunc(Display);  // Register idle function for continuous animation
     // Enter the main loop
    glutMainLoop();

    Mix_FreeChunk(backgroundSound);
    Mix_FreeChunk(collisionSound); // Free the collision sound effect
    Mix_FreeChunk(winSound);
    Mix_FreeChunk(loseSound);
    Mix_CloseAudio();
    SDL_Quit();

    return 0;
}