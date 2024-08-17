#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#define screenHeight 480
#define screenWidth 640
#define frames 144

void must_init(bool test, const char *description)
{
    if (test)
        return;

    printf("couldn't initialize %s\n", description);
    exit(1);
}

bool collide(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
{
    if (ax1 > bx2)
        return false;
    if (ax2 < bx1)
        return false;
    if (ay1 > by2)
        return false;
    if (ay2 < by1)
        return false;

    return true;
}

typedef struct Paletka
{
    int posX, posY;
    int velocity;
    int score;
    int width;
    int height;
} Paletka;

typedef struct Ball
{
    int posX, posY;
    int velX, velY;
    int radius;
    int bounce_counter;
} Ball;

enum GameState
{
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER
};

void initPlayer(Paletka *player, int posX, int posY, int width, int height, int velocity)
{
    player->posX = posX;
    player->posY = posY;
    player->width = width;
    player->height = height;
    player->velocity = velocity;
    player->score = 0;
}

void initBall(Ball *ball, int posX, int posY, int velX, int velY, int radius, int bounce_counter)
{
    ball->posX = posX;
    ball->posY = posY;
    ball->velX = velX;
    ball->velY = velY;
    ball->radius = radius;
    ball->bounce_counter = bounce_counter;
}

void updatePlayer(Paletka *player, int direction, float deltaTime)
{
    player->posY += direction * player->velocity * deltaTime;

    // Prevent paddles from going out of bounds
    if (player->posY < 0)
        player->posY = 0;
    if (player->posY + player->height > screenHeight)
        player->posY = screenHeight - player->height;
}

void drawGame(const Paletka *player1, const Paletka *player2, const Ball *ball, ALLEGRO_FONT *font)
{
    // Display scores
    al_draw_textf(font, al_map_rgb(255, 255, 255), screenWidth / 2, 10, ALLEGRO_ALIGN_CENTER, "PlayerOne: %d PlayerTwo: %d BallBounce: %d", player1->score, player2->score, ball->bounce_counter);

    // Draw paddles
    al_draw_filled_rectangle(player1->posX, player1->posY, player1->posX + player1->width, player1->posY + player1->height, al_map_rgb(255, 0, 0));
    al_draw_filled_rectangle(player2->posX, player2->posY, player2->posX + player2->width, player2->posY + player2->height, al_map_rgb(0, 0, 255));

    // Draw the ball
    al_draw_filled_circle(ball->posX, ball->posY, ball->radius, al_map_rgb(255, 255, 0));
}
void handleBallMovementAndCollision(Ball *ball, Paletka *player1, Paletka *player2, GameState *gameState)
{
    if (ball->bounce_counter > 0 && ball->bounce_counter % 2 == 0)
    {
        // Increase only the X velocity for a better effect
        if (ball->velX > 0)
            ball->velX++;
        else
            ball->velX--;

        // Optionally, limit the maximum speed to prevent the game from becoming too fast
        if (abs(ball->velX) > 10)
        {
            ball->velX = (ball->velX > 0) ? 10 : -10;
        }
    }

    // Move the ball
    ball->posX += ball->velX;
    ball->posY += ball->velY;

    // Check for collision with top and bottom walls
    if (ball->posY - ball->radius <= 0 || ball->posY + ball->radius >= screenHeight)
    {
        ball->velY = -ball->velY; // Reverse Y direction
    }

    // Check for collision with left paddle (PlayerOne)
    if (collide(
            player1->posX, player1->posY,
            player1->posX + player1->width, player1->posY + player1->height,
            ball->posX - ball->radius, ball->posY - ball->radius,
            ball->posX + ball->radius, ball->posY + ball->radius))
    {
        ball->bounce_counter++; // Reverse X direction
        ball->velX = -ball->velX;
    }

    // Check for collision with right paddle (PlayerTwo)
    if (collide(
            player2->posX, player2->posY,
            player2->posX + player2->width, player2->posY + player2->height,
            ball->posX - ball->radius, ball->posY - ball->radius,
            ball->posX + ball->radius, ball->posY + ball->radius))
    {
        ball->bounce_counter++; // Reverse X direction
        ball->velX = -ball->velX;
    }

    // Check for goals (if the ball crosses the left or right edges)
    if (ball->posX - ball->radius <= 0)
    {
        // PlayerTwo scores
        player2->score++;
        initBall(ball, screenWidth / 2, screenHeight / 2, 2, 2, 5, 0); // Reset ball
        *gameState = STATE_MENU;                                       // Go back to menu
    }
    else if (ball->posX + ball->radius >= screenWidth)
    {
        // PlayerOne scores
        player1->score++;
        initBall(ball, screenWidth / 2, screenHeight / 2, 2, 2, 5, 0); // Reset ball
        *gameState = STATE_MENU;                                       // Go back to menu
    }
}
void computerMovement(Ball *pileczka, Paletka *computer)
{
    if (pileczka->posY > computer->posY + computer->height / 2)
    {
        updatePlayer(computer, 1, 1.0f);
    }
    else
    {
        updatePlayer(computer, -1, 1.0f);
    }
}

int main()
{
    must_init(al_init(), "allegro");
    must_init(al_install_keyboard(), "keyboard");

    ALLEGRO_TIMER *timer = al_create_timer(1.0 / frames);
    must_init(timer, "timer");

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    must_init(queue, "queue");

    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

    ALLEGRO_DISPLAY *disp = al_create_display(screenWidth, screenHeight);
    must_init(disp, "display");

    ALLEGRO_FONT *font = al_create_builtin_font();
    must_init(font, "font");

    must_init(al_init_primitives_addon(), "primitives");

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    bool done = false;
    bool redraw = true;
    ALLEGRO_EVENT event;

    // Initialize PlayerOne and PlayerTwo
    Paletka PlayerOne, PlayerTwo;
    initPlayer(&PlayerOne, 10, screenHeight / 2 - 60, 10, 120, 5);               // Player on the left
    initPlayer(&PlayerTwo, screenWidth - 20, screenHeight / 2 - 60, 10, 120, 5); // Player on the right

    // Initialize the Ball
    Ball Pileczka;
    initBall(&Pileczka, screenWidth / 2, screenHeight / 2, 2, 2, 5, 0);

    enum GameState gameState = STATE_MENU;

#define KEY_SEEN 1
#define KEY_RELEASED 2

    unsigned char key[ALLEGRO_KEY_MAX];
    memset(key, 0, sizeof(key));
    al_start_timer(timer);

    while (!done)
    {

        al_wait_for_event(queue, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_TIMER:
            if (gameState == STATE_MENU)
            {
                if (key[ALLEGRO_KEY_SPACE])
                {
                    gameState = STATE_PLAYING;
                }
            }
            if (gameState == STATE_PLAYING)
            {
                // Handle player movement
                // if (key[ALLEGRO_KEY_UP])
                // {
                //     computerMovement(&Pileczka, &PlayerTwo);
                //     updatePlayer(&PlayerTwo, -1, 1.0f);
                // }
                // if (key[ALLEGRO_KEY_DOWN])
                // {
                //     computerMovement(&Pileczka, &PlayerTwo);
                //     updatePlayer(&PlayerTwo, 1, 1.0f);
                // }
                if (key[ALLEGRO_KEY_W])
                    updatePlayer(&PlayerOne, -1, 1.0f);
                if (key[ALLEGRO_KEY_S])
                    updatePlayer(&PlayerOne, 1, 1.0f);

                // updateBall(&Pileczka, &PlayerOne, &PlayerTwo, &gameState); // Update ball movement
                // ball_bounce(&Pileczka, &PlayerOne, &PlayerTwo);
                handleBallMovementAndCollision(&Pileczka, &PlayerOne, &PlayerTwo, &gameState);
                computerMovement(&Pileczka, &PlayerTwo);
            }

            if (key[ALLEGRO_KEY_ESCAPE])
                done = true;

            for (int i = 0; i < ALLEGRO_KEY_MAX; i++)
                key[i] &= KEY_SEEN;

            redraw = true;
            break;

        case ALLEGRO_EVENT_KEY_DOWN:
            key[event.keyboard.keycode] = KEY_SEEN | KEY_RELEASED;
            break;
        case ALLEGRO_EVENT_KEY_UP:
            key[event.keyboard.keycode] &= KEY_RELEASED;
            break;

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
        }

        if (redraw && al_is_event_queue_empty(queue))
        {
            al_clear_to_color(al_map_rgb(0, 0, 0));
            drawGame(&PlayerOne, &PlayerTwo, &Pileczka, font);
            al_flip_display();

            redraw = false;
        }
    }

    al_destroy_font(font);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}
