#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <Windows.h>
#include <SDL_mixer.h>


// Variables globales pour gérer le mode AFK
SDL_bool afkModeActive = SDL_FALSE;
SDL_Thread* afkThread = NULL;
SDL_bool afkRequestedStop = SDL_FALSE;

// Fonction pour quitter en cas d'erreur
void SDL_ExitWithError(const char *message) {
    SDL_Log("Erreur : %s > %s\n", message, SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
}

// Fonction pour charger et afficher le logo en utilisant SDL_LoadBMP (uniquement pour BMP)
SDL_Texture* loadLogo(SDL_Renderer* renderer, const char* imagePath, int* width, int* height) {
    SDL_Surface* logoSurface = SDL_LoadBMP(imagePath);
    if (!logoSurface) {
        SDL_ExitWithError("Impossible de charger l'image du logo");
    }

    SDL_Texture* logoTexture = SDL_CreateTextureFromSurface(renderer, logoSurface);
    SDL_FreeSurface(logoSurface);  // Libération de la surface après création de la texture

    if (!logoTexture) {
        SDL_ExitWithError("Impossible de créer la texture du logo");
    }

    *width = logoSurface->w;
    *height = logoSurface->h;

    return logoTexture;
}

// Fonction pour gérer le mode AFK
int AfkMode(void* data)
{
    INPUT input[1];

    for (int i = 0; i < 10; i++)
    {
        if (afkRequestedStop) {
            SDL_Log("AFK stoppé manuellement !");
            break;
        }

        // Simuler touches & clics
        ZeroMemory(&input, sizeof(INPUT));

        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = 'Z';
        SendInput(1, input, sizeof(INPUT));
        Sleep(1000);

        input[0].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, input, sizeof(INPUT));
        Sleep(2000);

        if (afkRequestedStop) break;

        input[0].ki.dwFlags = 0;
        input[0].ki.wVk = 'S';
        SendInput(1, input, sizeof(INPUT));
        Sleep(1000);

        input[0].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, input, sizeof(INPUT));
        Sleep(2000);

        if (afkRequestedStop) break;

        input[0].ki.wVk = VK_SPACE;
        input[0].ki.dwFlags = 0;
        SendInput(1, input, sizeof(INPUT));
        Sleep(1000);

        input[0].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, input, sizeof(INPUT));
        Sleep(1000);

        input[0].type = INPUT_MOUSE;
        input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, input, sizeof(INPUT));
        Sleep(1000);
        input[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, input, sizeof(INPUT));
        Sleep(3000);
    }
    return 0;
}
// Structure pour un bouton avec texte et couleur
typedef struct {
    SDL_Rect rect;
    SDL_Texture* texture;
    SDL_Color color;  // Couleur du bouton
} Button;

// Fonction pour créer un bouton
Button createButton(SDL_Renderer* renderer, TTF_Font* font, const char* label, SDL_Color textColor, SDL_Color buttonColor, int x, int y, int width, int height) {
    Button button;
    button.rect.x = x;
    button.rect.y = y;
    button.rect.w = width;
    button.rect.h = height;
    button.color = buttonColor;  // Initialiser la couleur du bouton

    // Créer une surface avec le texte et la couleur
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, label, textColor);
    if (textSurface == NULL) SDL_ExitWithError("Impossible de créer la surface de texte");

    // Créer une texture à partir de la surface
    button.texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface); // Libération de la surface après création de la texture

    return button;
}

// Fonction pour dessiner un bouton
void drawButton(SDL_Renderer* renderer, Button button) {
    // Dessiner le rectangle de fond du bouton avec la couleur spécifiée
    SDL_SetRenderDrawColor(renderer, button.color.r, button.color.g, button.color.b, button.color.a);
    SDL_RenderFillRect(renderer, &button.rect);

    // Dessiner le texte sur le bouton
    int textWidth = 0, textHeight = 0;
    SDL_QueryTexture(button.texture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect textRect = {button.rect.x + (button.rect.w - textWidth) / 2, button.rect.y + (button.rect.h - textHeight) / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, button.texture, NULL, &textRect);
}

// Fonction pour vérifier si le bouton a été cliqué
int isButtonClicked(Button button, int mouseX, int mouseY) {
    return (mouseX >= button.rect.x && mouseX <= button.rect.x + button.rect.w &&
            mouseY >= button.rect.y && mouseY <= button.rect.y + button.rect.h);
}

int main(int argc, char **argv)
{
    SDL_Window *Window = NULL;
    SDL_Renderer *renderer = NULL;
    TTF_Font *font = NULL;

    // Couleurs
    SDL_Color textColor = {255, 255, 255}; // Texte blanc
    SDL_Color buttonColor = {0, 0, 155};  // Couleur du bouton
    SDL_Color windowBackgroundColor = {70, 70, 70, 255}; // Gris clair (fond de la fenêtre)

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_ExitWithError("Erreur SDL_mixer");
    }
    Mix_Chunk* clickSound = Mix_LoadWAV("dog.wav");
    if (clickSound == NULL) {
    SDL_ExitWithError("Erreur chargement son");
}


    // Créer un bouton centré
    Button button;
    SDL_bool program_launched = SDL_TRUE;

    // Initialisation SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) SDL_ExitWithError("Initialisation SDL");

    // Initialisation SDL_ttf
    if (TTF_Init() == -1) SDL_ExitWithError("Erreur d'initialisation de SDL_ttf");

    // Création de la fenêtre et du renderer
    if(SDL_CreateWindowAndRenderer(500, 300, 0, &Window, &renderer) != 0) SDL_ExitWithError("Impossible de créer la fenêtre et le rendu");
    SDL_SetWindowTitle(Window, "Patrol Bot - By Comet");
    // Charger l'image BMP pour l'icône
    SDL_Surface* icon = SDL_LoadBMP("patpat.bmp");
    if (icon == NULL) {
       SDL_Log("Erreur chargement icône : %s", SDL_GetError());
    } else {
       SDL_SetWindowIcon(Window, icon); // Appliquer l'icône à la fenêtre
       SDL_FreeSurface(icon); // Libération après l'avoir appliquée
    }

    // Chargement de la police
    font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 24);
    if (font == NULL) SDL_ExitWithError("Impossible de charger la police");

    // Charger le logo
    int logoWidth = 0, logoHeight = 0;
    SDL_Texture* logoTexture = loadLogo(renderer, "patpat.bmp", &logoWidth, &logoHeight);  // Charger logo.bmp

    // Calculer la position du logo
    int logoX = (500 - logoWidth) / 2;
    int logoY = 20;

    // Créer un bouton centré
    int buttonWidth = 200, buttonHeight = 50;
    int buttonX = (500 - buttonWidth) / 2;
    int buttonY = (400 - buttonHeight) / 2;
    button = createButton(renderer, font, "Active", textColor, buttonColor, buttonX, buttonY, buttonWidth, buttonHeight);

    // Boucle principale
    while(program_launched)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    program_launched = SDL_FALSE;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        // Vérifier si le bouton a été cliqué
                        if (isButtonClicked(button, event.button.x, event.button.y))
                        {
                            if(!afkModeActive)  // Si le mode AFK n'est pas actif
                            {
                                afkModeActive = SDL_TRUE;
                                Mix_PlayChannel(-1, clickSound, 0); // -1 = premier canal libre, 0 = pas de boucle
                                button.color = (SDL_Color){255, 0, 0}; // Rouge pour indiquer que AFK est activé
                                button = createButton(renderer, font, "Desactive", textColor, buttonColor, buttonX, buttonY, buttonWidth, buttonHeight);
                                button.color = (SDL_Color){0, 0, 55};
                                afkRequestedStop = SDL_FALSE;

                                // Créer un thread pour exécuter le mode AFK
                                afkThread = SDL_CreateThread(AfkMode, "afkThread", NULL);
                                SDL_Log("AFK Mode activé !");
                            }
                            else  // Si le mode AFK est déjà actif
                            {
                                SDL_Log("AFK Mode désactivé !");
                                afkRequestedStop = SDL_TRUE;
                                button.color = (SDL_Color){50, 50, 50}; // Retour à la couleur initiale du bouton
                                button = createButton(renderer, font, "Active", textColor, buttonColor, buttonX, buttonY, buttonWidth, buttonHeight);
                                afkModeActive = SDL_FALSE;
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        // Changer la couleur de fond de la fenêtre
        SDL_SetRenderDrawColor(renderer, windowBackgroundColor.r, windowBackgroundColor.g, windowBackgroundColor.b, windowBackgroundColor.a);
        SDL_RenderClear(renderer); // Appliquer le fond coloré

        // Dessiner le logo
        SDL_Rect logoRect = {logoX, logoY, logoWidth, logoHeight};
        SDL_RenderCopy(renderer, logoTexture, NULL, &logoRect);

        // Dessiner le bouton
        drawButton(renderer, button);

        // Afficher le texte "By Comet" en bas de la fenêtre
        SDL_Color cometColor = {0, 0, 155, 255};
        SDL_Surface* cometSurface = TTF_RenderText_Solid(font, "By Comet", cometColor);
        SDL_Texture* cometTexture = SDL_CreateTextureFromSurface(renderer, cometSurface);

        SDL_Rect cometRect;
        cometRect.w = cometSurface->w;
        cometRect.h = cometSurface->h;
        cometRect.x = 360 - cometRect.w - 10;
        cometRect.y = 140; // Positionner en bas

        SDL_RenderCopy(renderer, cometTexture, NULL, &cometRect);

        SDL_FreeSurface(cometSurface);
        SDL_DestroyTexture(cometTexture);

        // Mettre à jour l'écran
        SDL_RenderPresent(renderer);

        // Limiter à 60 FPS
        SDL_Delay(16);
    }

    // Libérer les ressources
    Mix_FreeChunk(clickSound);
    Mix_CloseAudio();
    SDL_DestroyTexture(button.texture);
    SDL_DestroyTexture(logoTexture);  // Libérer la texture du logo
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(Window);

    // Quitter SDL et SDL_ttf
    TTF_Quit();
    SDL_Quit();

    return 0;
}
