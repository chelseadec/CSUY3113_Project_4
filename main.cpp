/**
* Author: Chelsea DeCambre
* Assignment: Rise of the AI
* Date due: 2023-07-23, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 13
#define ENEMY_COUNT 3

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity *player;
    Entity *platforms;
    Entity *enemies;
    
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
};


// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.127f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.359f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

bool is_game_over = false;
bool auto_end     = false;

const char SPRITESHEET_FILEPATH[] = "/Users/chelsea/Desktop/Enemy AI/SDLProject/assets/george_0.png",
           PLATFORM_FILEPATH[]    = "/Users/chelsea/Desktop/Enemy AI/SDLProject/assets/platformPack_tile027.png",
           ENEMY_FILEPATH[]       = "/Users/chelsea/Desktop/Enemy AI/SDLProject/assets/soph.png",
           FONT_FILEPATH[]        = "/Users/chelsea/Desktop/Enemy AI/SDLProject/assets/font1.png";

const char BGM_FILEPATH[] = "/Users/chelsea/Desktop/Enemy AI/SDLProject/assets/dooblydoo.mp3",
           SFX_FILEPATH[] = "/Users/chelsea/Desktop/Enemy AI/SDLProject/assets/bounce.wav";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

const float PLATFORM_OFFSET = 5.0f;

// ––––– VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

// ––––– GENERAL FUNCTIONS ––––– //

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}


void initialise()
{
    // ––––– GENERAL STUFF ––––– //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Hello, AI!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    // ––––– VIDEO STUFF ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.SetProjectionMatrix(g_projection_matrix);
    g_program.SetViewMatrix(g_view_matrix);
    
    glUseProgram(g_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ––––– PLATFORM ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    
    g_state.platforms = new Entity[PLATFORM_COUNT];
    
    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        g_state.platforms[i].set_entity_type(PLATFORM);
        g_state.platforms[i].m_texture_id = platform_texture_id;
        g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_OFFSET, -3.0f, 0.0f));
        g_state.platforms[i].set_width(0.4f);
        g_state.platforms[i].update(0.0f, NULL, NULL, 0);
        
        if (i > 11){
            g_state.platforms[i].set_entity_type(PLATFORM);
            g_state.platforms[i].m_texture_id = platform_texture_id;
            g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_OFFSET - 6, -2.0f, 0.0f));
            g_state.platforms[i].set_width(0.4f);
            g_state.platforms[i].update(0.0f, NULL, NULL, 0);
        }
    }
    
    // ––––– GEORGE ––––– //
    // Existing
    g_state.player = new Entity();
    g_state.player->set_entity_type(PLAYER);
    g_state.player->set_position(glm::vec3(-2.0f, 0.0f, 0.0f));
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->set_speed(2.5f);
    g_state.player->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
    // Walking
    g_state.player->m_walking[g_state.player->LEFT]  = new int[4] { 1, 5, 9,  13 };
    g_state.player->m_walking[g_state.player->RIGHT] = new int[4] { 3, 7, 11, 15 };
    g_state.player->m_walking[g_state.player->UP]    = new int[4] { 2, 6, 10, 14 };
    g_state.player->m_walking[g_state.player->DOWN]  = new int[4] { 0, 4, 8,  12 };

    g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];  // start George looking right
    g_state.player->m_animation_frames = 4;
    g_state.player->m_animation_index  = 0;
    g_state.player->m_animation_time   = 0.0f;
    g_state.player->m_animation_cols   = 4;
    g_state.player->m_animation_rows   = 4;
    g_state.player->set_height(1.0f);
    g_state.player->set_width(1.0f);
    
    // Jumping
    g_state.player->set_jumping_power(5.0f);
    
    // ––––– SOPHIE ––––– //
    GLuint enemy_texture_id = load_texture(ENEMY_FILEPATH);
    
    g_state.enemies = new Entity[ENEMY_COUNT];
    for (int i = 0; i < ENEMY_COUNT; ++i) {
        g_state.enemies[i].set_entity_type(ENEMY);
        
        
        if (i == 0) {
           g_state.enemies[i].set_ai_type(GUARD);
           g_state.enemies[i].set_ai_state(IDLE);
           g_state.enemies[i].m_texture_id = enemy_texture_id;
           g_state.enemies[i].set_position(glm::vec3(-5.0f, 0.0f, 0.0f));
           g_state.enemies[i].set_movement(glm::vec3(0.0f));
           g_state.enemies[i].set_speed(1.0f);
           g_state.enemies[i].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
        }
        else if (i == 1) {
            g_state.enemies[i].set_ai_type(WALKER);
            g_state.enemies[i].set_ai_state(IDLE);
            g_state.enemies[i].m_texture_id = enemy_texture_id;
            g_state.enemies[i].set_position(glm::vec3(0.0f, 0.0f, 0.0f));
            g_state.enemies[i].set_movement(glm::vec3(0.0f));
            g_state.enemies[i].set_speed(0.3f);
            g_state.enemies[i].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
        }
        else {
            g_state.enemies[i].set_ai_type(GUARD);
            g_state.enemies[i].set_ai_state(IDLE);
            g_state.enemies[i].m_texture_id = enemy_texture_id;
            g_state.enemies[i].set_position(glm::vec3(1.0f, -1.0f, 0.0f));
            g_state.enemies[i].set_movement(glm::vec3(0.0f));
            g_state.enemies[i].set_speed(2.0f);
            g_state.enemies[i].set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
        }
        g_state.enemies[i].update(0.0f, g_state.player, g_state.player, 1);
        
//        g_state.enemies[i].set_ai_state(IDLE);
//        g_state.enemies[i].m_texture_id = enemy_texture_id;
////        g_state.enemies[i].set_position(glm::vec3(3.0f, 0.0f, 0.0f));
//        g_state.enemies[i].set_movement(glm::vec3(0.0f));
//        g_state.enemies[i].set_speed(1.0f);
//        g_state.enemies[i].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
        
        
    }
    
    // ––––– TEXT ––––– //
    
    

    
    // ––––– AUDIO STUFF ––––– //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    
    g_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 4.0f);
    
    g_state.jump_sfx = Mix_LoadWAV(SFX_FILEPATH);
    
    // ––––– GENERAL STUFF ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        // Jump
                        if (g_state.player->m_collided_bottom)
                        {
                            g_state.player->m_is_jumping = true;
                            Mix_PlayChannel(-1, g_state.jump_sfx, 0);
                            
//                            for (int i = 0; i < ENEMY_COUNT; ++i) {
//                                g_state.enemies[i].check_collision_y(g_state.player, 1);
//                                if (g_state.enemies[i].m_collided_top){
//                                    LOG("trueeeeeeeeeee");
////                                    delete[] g_state.enemies;
//                                }
//                            }
                        }
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->m_movement.x = -1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->m_movement.x = 1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];
    }
    
    if (glm::length(g_state.player->m_movement) > 1.0f)
    {
        g_state.player->m_movement = glm::normalize(g_state.player->m_movement);
    }
}



const int FONTBANK_SIZE = 16;

void DrawText(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->SetModelMatrix(model_matrix);
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void update()
{
    
    g_state.player->check_collision_y(g_state.enemies, ENEMY_COUNT);
    g_state.enemies->check_collision_x(g_state.player, 1);
    
    for (int i = 0; i < ENEMY_COUNT; ++i) {
        if (g_state.enemies[i].m_collided_right || g_state.enemies[i].m_collided_left){
            auto_end = true;
        }
    }
    
    if (g_state.player->m_collided_bottom){
        LOG("COLLLLIDDEEDDDDD!!!!!!");
    }
    int count = 0;
    for (int i = 0; i < ENEMY_COUNT; ++i) {
        if (!g_state.enemies[i].get_is_active()){
            ++count;
        }
    }

    if (count == ENEMY_COUNT && !is_game_over) {
        is_game_over = true;
    }

    
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP) {
        g_state.player->update(FIXED_TIMESTEP, g_state.player, g_state.platforms, PLATFORM_COUNT);
        
        for (int i = 0; i < ENEMY_COUNT; i++) {
            LOG("here ");
            LOG(i);
//            if (g_state.enemies[i].get_ai_type() == FLOAT) {
//                g_state.enemies[i].ai_float();
//            }
//            g_state.enemies[i].update(<#float delta_time#>, <#Entity *player#>, <#Entity *collidable_entities#>, <#int collidable_entity_count#>)
            g_state.enemies[i].update(FIXED_TIMESTEP, g_state.player, g_state.platforms, PLATFORM_COUNT);
        }
        g_state.player->update(FIXED_TIMESTEP, g_state.player, g_state.enemies, ENEMY_COUNT);
//        g_state.enemies->update(FIXED_TIMESTEP, g_state.enemies, g_state.player, 1);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
}


void render()
{
    GLuint font_texture_id = load_texture(FONT_FILEPATH);
    glClear(GL_COLOR_BUFFER_BIT);
        
    g_state.player->render(&g_program);
    
    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
    for (int i = 0; i < ENEMY_COUNT; i++)    g_state.enemies[i].render(&g_program);

    // Draw "YOU WIN!" text if the game is over
    if (is_game_over) {
        DrawText(&g_program, font_texture_id, "YOU WIN!", 0.5f, 0.10f, glm::vec3(-2.0f, 0.0f, 0.0f));
    }
    else if (auto_end || !g_state.player->get_is_active()) {
        LOG("ENNDDDDDDDDDDD");
        DrawText(&g_program, font_texture_id, "YOU LOSE!", 0.5f, 0.10f, glm::vec3(-2.25f, 0.0f, 0.0f));

    }
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_state.platforms;
    delete [] g_state.enemies;
    delete    g_state.player;
    Mix_FreeChunk(g_state.jump_sfx);
    Mix_FreeMusic(g_state.bgm);
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
