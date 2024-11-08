#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

#define LARGURA_TELA 800
#define ALTURA_TELA 600
#define LARGURA_LIXO 20
#define ALTURA_LIXO 20
#define VELOCIDADE_LIXO 1
#define VELOCIDADE_TIRO 5
#define DELAY_TIRO 1000  // Delay de 1 segundo em milissegundos
#define VIDA_MAXIMA 100
#define MAX_PONTUACOES 100

// Definir danos por tipo de lixo
#define DANO_BAIXO 5    // 🥤, 🧃
#define DANO_MEDIO 10   // 📦, 👟, 👕, 🎒
#define DANO_ALTO 15    // 🛞, 🪑
#define DANO_MUITO_ALTO 20 // 🚽

// Definir cores
SDL_Color corRio = {0, 153, 153, 255};  // Cor do rio entre azul e verde (ciano)
SDL_Color corMato = {34, 139, 34, 255}; // Verde para as margens (mato)
SDL_Color corPonte = {169, 169, 169, 255};  // Cor da ponte (cinza)

// Variáveis globais
SDL_Window *tela = NULL;
SDL_Renderer *renderizador = NULL;
SDL_Texture *texturaCanhao = NULL;
SDL_Texture *texturaTiro = NULL;
TTF_Font *fontePontuacao = NULL;

int vidaAtual = VIDA_MAXIMA;
int lixosDestruidos = 0;

// Declaração antecipada das estruturas e funções necessárias
typedef struct Lixo {
    int x, y;
    int largura, altura;
    SDL_Texture *textura;
    int dano;  // Dano que este tipo de lixo causa
    struct Lixo *proximo;
} Lixo;

typedef struct Tiro {
    float x, y;
    float vx, vy;
    struct Tiro *proximo;
} Tiro;

// Listas de lixos e tiros
Lixo *listaLixos = NULL;
Tiro *listaTiros = NULL;

// Declaração antecipada das funções necessárias
void loopJogo(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades);
SDL_Texture* carregarEmoji(SDL_Renderer *renderer, TTF_Font *font, const char *emoji);
void adicionarLixos(SDL_Texture *texturas[], int *probabilidades, int numTexturas, int totalProbabilidades);
void tiros(float inicioX, float inicioY, float destinoX, float destinoY);
void atualizarEDesenharLixos();
bool verificarColisao(SDL_Rect a, SDL_Rect b);
void atualizarEDesenharTiros();
bool inicializarSDL();
bool carregarMidia(SDL_Texture **texturasLixo, int numTexturas);
void desenharCena();
void desenharBarraDeVida();
void desenharMenu(int recorde);
void desenharPontuacao();
void loopMenu(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades);
void fecharSDL(SDL_Texture **texturasLixo, int numTexturas);
void salvarPontuacao(int novaPontuacao);
int lerRecorde();

// Função para desenhar a pontuação de lixos destruídos
void desenharPontuacao() {
    SDL_Color branco = {255, 255, 255, 255};
    char texto[50];
    snprintf(texto, sizeof(texto), "Lixos Destruidos: %d", lixosDestruidos);

    SDL_Surface *surfaceTexto = TTF_RenderText_Blended(fontePontuacao, texto, branco);
    SDL_Texture *texturaTexto = SDL_CreateTextureFromSurface(renderizador, surfaceTexto);

    SDL_Rect posicaoTexto = {10, ALTURA_TELA - 40, surfaceTexto->w, surfaceTexto->h};
    SDL_RenderCopy(renderizador, texturaTexto, NULL, &posicaoTexto);

    SDL_FreeSurface(surfaceTexto);
    SDL_DestroyTexture(texturaTexto);
}

// Função para salvar a pontuação no arquivo e ordenar
void salvarPontuacao(int novaPontuacao) {
    int pontuacoes[MAX_PONTUACOES];
    int numPontuacoes = 0;

    FILE *arquivo = fopen("pontuacoes.txt", "r");
    if (arquivo) {
        while (fscanf(arquivo, "%d", &pontuacoes[numPontuacoes]) != EOF && numPontuacoes < MAX_PONTUACOES) {
            numPontuacoes++;
        }
        fclose(arquivo);
    }

    if (numPontuacoes < MAX_PONTUACOES) {
        pontuacoes[numPontuacoes++] = novaPontuacao;
    }

    for (int i = 0; i < numPontuacoes - 1; i++) {
        for (int j = 0; j < numPontuacoes - i - 1; j++) {
            if (pontuacoes[j] < pontuacoes[j + 1]) {
                int temp = pontuacoes[j];
                pontuacoes[j] = pontuacoes[j + 1];
                pontuacoes[j + 1] = temp;
            }
        }
    }

    arquivo = fopen("pontuacoes.txt", "w");
    if (arquivo) {
        for (int i = 0; i < numPontuacoes; i++) {
            fprintf(arquivo, "%d\n", pontuacoes[i]);
        }
        fclose(arquivo);
    }
}

// Função para ler o recorde do arquivo
int lerRecorde() {
    int recorde = 0;
    FILE *arquivo = fopen("pontuacoes.txt", "r");
    if (arquivo) {
        fscanf(arquivo, "%d", &recorde);
        fclose(arquivo);
    }
    return recorde;
}

void desenharMenu(int recorde) {
    SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
    SDL_RenderClear(renderizador);

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 48);
    SDL_Color branco = {255, 255, 255, 255};

    SDL_Surface *textoIniciarSurface = TTF_RenderUTF8_Blended(font, "Iniciar Jogo", branco);
    SDL_Texture *textoIniciar = SDL_CreateTextureFromSurface(renderizador, textoIniciarSurface);
    SDL_Surface *textoSairSurface = TTF_RenderUTF8_Blended(font, "Sair", branco);
    SDL_Texture *textoSair = SDL_CreateTextureFromSurface(renderizador, textoSairSurface);

    char recordeTexto[50];
    snprintf(recordeTexto, sizeof(recordeTexto), "Recorde: %d", recorde);
    SDL_Surface *textoRecordeSurface = TTF_RenderText_Blended(font, recordeTexto, branco);
    SDL_Texture *textoRecorde = SDL_CreateTextureFromSurface(renderizador, textoRecordeSurface);

    SDL_Rect rectIniciar = {LARGURA_TELA / 2 - 100, ALTURA_TELA / 2 - 60, 200, 50};
    SDL_Rect rectSair = {LARGURA_TELA / 2 - 50, ALTURA_TELA / 2 + 20, 100, 50};
    SDL_Rect rectRecorde = {LARGURA_TELA / 2 - 100, ALTURA_TELA / 2 - 120, 200, 50};

    SDL_RenderCopy(renderizador, textoIniciar, NULL, &rectIniciar);
    SDL_RenderCopy(renderizador, textoSair, NULL, &rectSair);
    SDL_RenderCopy(renderizador, textoRecorde, NULL, &rectRecorde);

    SDL_RenderPresent(renderizador);

    SDL_FreeSurface(textoIniciarSurface);
    SDL_FreeSurface(textoSairSurface);
    SDL_FreeSurface(textoRecordeSurface);
    SDL_DestroyTexture(textoIniciar);
    SDL_DestroyTexture(textoSair);
    SDL_DestroyTexture(textoRecorde);
    TTF_CloseFont(font);
}

void loopMenu(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades) {
    bool rodando = true;
    bool menuAtivo = true;
    SDL_Event evento;

    int recorde = lerRecorde();

    while (rodando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            } else if (evento.type == SDL_MOUSEBUTTONDOWN && menuAtivo) {
                int mouseX = evento.button.x;
                int mouseY = evento.button.y;

                if (mouseX >= LARGURA_TELA / 2 - 100 && mouseX <= LARGURA_TELA / 2 + 100 && 
                    mouseY >= ALTURA_TELA / 2 - 60 && mouseY <= ALTURA_TELA / 2 - 10) {
                    // Iniciar Jogo
                    menuAtivo = false;
                    loopJogo(texturasLixo, probabilidades, numTexturas, totalProbabilidades);
                } else if (mouseX >= LARGURA_TELA / 2 - 50 && mouseX <= LARGURA_TELA / 2 + 50 && 
                           mouseY >= ALTURA_TELA / 2 + 20 && mouseY <= ALTURA_TELA / 2 + 70) {
                    // Sair
                    rodando = false;
                }
            }
        }

        if (menuAtivo) {
            desenharMenu(recorde);
        }
    }
}

void loopJogo(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades) {
    bool rodando = true;
    SDL_Event evento;

    int canhaoX = LARGURA_TELA / 2;
    int canhaoY = ALTURA_TELA - 50;
    Uint32 tempoUltimoTiro = 0;  // Variável para controlar o tempo do último tiro

    while (rodando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            } else if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) {
                Uint32 tempoAtual = SDL_GetTicks();
                if (tempoAtual - tempoUltimoTiro >= DELAY_TIRO) {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    tiros(canhaoX, canhaoY, mouseX, mouseY);
                    tempoUltimoTiro = tempoAtual;
                }
            }
        }

        if (rand() % 50 == 0) {
            adicionarLixos(texturasLixo, probabilidades, numTexturas, totalProbabilidades);
        }

        SDL_RenderClear(renderizador);

        desenharCena();
        desenharBarraDeVida();
        atualizarEDesenharLixos();
        atualizarEDesenharTiros();
        desenharPontuacao();

        if (vidaAtual <= 0) {
            SDL_Log("Fim de jogo! O mar foi poluído demais.");
            salvarPontuacao(lixosDestruidos);
            rodando = false;
        }

        SDL_RenderPresent(renderizador);
        SDL_Delay(16);
    }
}

bool inicializarSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Erro ao inicializar SDL: %s", SDL_GetError());
        return false;
    }
    
    tela = SDL_CreateWindow("Rio com Ponte", 
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                              LARGURA_TELA, ALTURA_TELA, SDL_WINDOW_SHOWN);
    if (!tela) {
        SDL_Log("Erro ao criar tela: %s", SDL_GetError());
        return false;
    }
    
    renderizador = SDL_CreateRenderer(tela, -1, SDL_RENDERER_ACCELERATED);
    if (!renderizador) {
        SDL_Log("Erro ao criar renderizador: %s", SDL_GetError());
        return false;
    }

    if (TTF_Init() == -1) {
        SDL_Log("Erro ao inicializar SDL_ttf: %s", TTF_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_WEBP) & (IMG_INIT_PNG | IMG_INIT_WEBP))) {
        SDL_Log("Erro ao inicializar SDL_image: %s", IMG_GetError());
        return false;
    }
    
    return true;
}

bool carregarMidia(SDL_Texture **texturasLixo, int numTexturas) {
    SDL_Surface *surfaceCarregada = IMG_Load("imagens/cannon-isolated-on-transparent-free-png.webp");
    if (!surfaceCarregada) {
        SDL_Log("Erro ao carregar imagem do canhão: %s", IMG_GetError());
        return false;
    }

    texturaCanhao = SDL_CreateTextureFromSurface(renderizador, surfaceCarregada);
    SDL_FreeSurface(surfaceCarregada);
    if (!texturaCanhao) {
        SDL_Log("Erro ao criar textura do canhão: %s", SDL_GetError());
        return false;
    }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf", 28); // Ajuste com uma fonte de emojis válida
    if (!font) {
        SDL_Log("Erro ao carregar fonte: %s", TTF_GetError());
        return false;
    }

    texturaTiro = carregarEmoji(renderizador, font, "💣");
    if (!texturaTiro) {
        SDL_Log("Erro ao criar textura do tiro: %s", SDL_GetError());
        TTF_CloseFont(font);
        return false;
    }

    const char* emojis[] = {"🥤", "🧃", "📦", "👟", "👕", "🎒", "🛞", "🪑", "🚽"};
    for (int i = 0; i < numTexturas; i++) {
        texturasLixo[i] = carregarEmoji(renderizador, font, emojis[i]);
        if (!texturasLixo[i]) {
            SDL_Log("Erro ao criar textura do emoji de lixo: %s", SDL_GetError());
            TTF_CloseFont(font);
            return false;
        }
    }

    // Inicializa a fonte para pontuação
    fontePontuacao = TTF_OpenFont("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 24);
    if (!fontePontuacao) {
        SDL_Log("Erro ao carregar fonte de pontuação: %s", TTF_GetError());
        return false;
    }

    TTF_CloseFont(font);
    return true;
}

void fecharSDL(SDL_Texture **texturasLixo, int numTexturas) {
    while (listaLixos != NULL) {
        Lixo *temp = listaLixos;
        listaLixos = listaLixos->proximo;
        free(temp);
    }

    while (listaTiros != NULL) {
        Tiro *temp = listaTiros;
        listaTiros = listaTiros->proximo;
        free(temp);
    }

    SDL_DestroyTexture(texturaCanhao);
    SDL_DestroyTexture(texturaTiro);

    for (int i = 0; i < numTexturas; i++) {
        SDL_DestroyTexture(texturasLixo[i]);
    }

    TTF_CloseFont(fontePontuacao);
    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(tela);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

SDL_Texture* carregarEmoji(SDL_Renderer *renderer, TTF_Font *font, const char *emoji) {
    SDL_Color branco = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, emoji, branco);
    SDL_Texture *textura = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return textura;
}

void adicionarLixos(SDL_Texture *texturas[], int *probabilidades, int numTexturas, int totalProbabilidades) {
    int x = (LARGURA_TELA / 6) + rand() % (LARGURA_TELA * 2 / 3);
    int sorteio = rand() % totalProbabilidades;

    int acumulador = 0;
    int index = 0;
    for (int i = 0; i < numTexturas; i++) {
        acumulador += probabilidades[i];
        if (sorteio < acumulador) {
            index = i;
            break;
        }
    }

    Lixo *novoLixo = (Lixo *)malloc(sizeof(Lixo));
    novoLixo->x = x;
    novoLixo->y = 0;
    novoLixo->textura = texturas[index];

    if (index == 2 || index == 6 || index == 7 || index == 8) {  // 📦, 🛞, 🪑, 🚽
        novoLixo->largura = LARGURA_LIXO * 2;
        novoLixo->altura = ALTURA_LIXO * 2;
    } else if(index == 4 || index == 5) { // 👕, 🎒
        novoLixo->largura = LARGURA_LIXO * 1.5;
        novoLixo->altura = ALTURA_LIXO * 1.5;
    } else if(index == 3) { // 👟
        novoLixo->largura = LARGURA_LIXO * 1.2;
        novoLixo->altura = ALTURA_LIXO * 1.2;
    } else {   // 🥤, 🧃
        novoLixo->largura = LARGURA_LIXO;
        novoLixo->altura = ALTURA_LIXO;
    }

        // Definir dano com base no tipo de lixo
    if (index == 0 || index == 1) {  // 🥤, 🧃
        novoLixo->dano = DANO_BAIXO;
    } else if (index == 2 || index == 3 || index == 4 || index == 5) {  // 📦, 👟, 👕, 🎒
        novoLixo->dano = DANO_MEDIO;
    } else if (index == 6 || index == 7) {  // 🛞, 🪑
        novoLixo->dano = DANO_ALTO;
    } else if (index == 8) {  // 🚽
        novoLixo->dano = DANO_MUITO_ALTO;
    }

    novoLixo->proximo = listaLixos;
    listaLixos = novoLixo;
}

void tiros(float inicioX, float inicioY, float destinoX, float destinoY) {
    Tiro *novoTiro = (Tiro *)malloc(sizeof(Tiro));
    novoTiro->x = inicioX;
    novoTiro->y = inicioY;

    // Calcular direção
    float dx = destinoX - inicioX;
    float dy = destinoY - inicioY;
    float comprimento = sqrtf(dx * dx + dy * dy);
    if (comprimento != 0) {
        novoTiro->vx = VELOCIDADE_TIRO * (dx / comprimento);
        novoTiro->vy = VELOCIDADE_TIRO * (dy / comprimento);
    } else {
        novoTiro->vx = 0;
        novoTiro->vy = -VELOCIDADE_TIRO; // Padrão para cima se não houver movimento
    }

    novoTiro->proximo = listaTiros;
    listaTiros = novoTiro;
}

void atualizarEDesenharLixos() {
    Lixo *atual = listaLixos;
    Lixo *anterior = NULL;

    while (atual != NULL) {
        atual->y += VELOCIDADE_LIXO;

        SDL_Rect retanguloLixo = {atual->x, atual->y, atual->largura, atual->altura};
        SDL_RenderCopy(renderizador, atual->textura, NULL, &retanguloLixo);

        if (atual->y > ALTURA_TELA) {
            // Reduz a vida de acordo com o dano do lixo
            vidaAtual -= atual->dano;
            if (vidaAtual < 0) {
                vidaAtual = 0;
            }

            if (anterior == NULL) {
                listaLixos = atual->proximo;
                free(atual);
                atual = listaLixos;
            } else {
                anterior->proximo = atual->proximo;
                free(atual);
                atual = anterior->proximo;
            }
        } else {
            anterior = atual;
            atual = atual->proximo;
        }
    }
}

bool verificarColisao(SDL_Rect a, SDL_Rect b) {
    return !(a.x + a.w < b.x || a.x > b.x + b.w || a.y + a.h < b.y || a.y > b.y + b.h);
}

void atualizarEDesenharTiros() {
    Tiro *atual = listaTiros;
    Tiro *anterior = NULL;

    while (atual != NULL) {
        atual->x += atual->vx;
        atual->y += atual->vy;

        SDL_Rect retanguloTiro = {(int)atual->x - 10 / 2, (int)atual->y, 20, 20};

        Lixo *lixoAtual = listaLixos;
        Lixo *lixoAnterior = NULL;
        bool colisaoDetectada = false;

        while (lixoAtual != NULL && !colisaoDetectada) {
            SDL_Rect retanguloLixo = {lixoAtual->x, lixoAtual->y, lixoAtual->largura, lixoAtual->altura};

            if (verificarColisao(retanguloTiro, retanguloLixo)) {
                lixosDestruidos++;  // Incrementa o contador de lixos destruídos
                if (lixoAnterior == NULL) {
                    listaLixos = lixoAtual->proximo;
                    free(lixoAtual);
                    lixoAtual = listaLixos;
                } else {
                    lixoAnterior->proximo = lixoAtual->proximo;
                    free(lixoAtual);
                    lixoAtual = lixoAnterior->proximo;
                }
                colisaoDetectada = true;
            } else {
                lixoAnterior = lixoAtual;
                lixoAtual = lixoAtual->proximo;
            }
        }

        if (colisaoDetectada) {
            if (anterior == NULL) {
                listaTiros = atual->proximo;
                free(atual);
                atual = listaTiros;
            } else {
                anterior->proximo = atual->proximo;
                free(atual);
                atual = anterior->proximo;
            }
        } else {
            SDL_RenderCopy(renderizador, texturaTiro, NULL, &retanguloTiro);

            if (atual->y < 0 || atual->y > ALTURA_TELA || atual->x < 0 || atual->x > LARGURA_TELA) {
                if (anterior == NULL) {
                    listaTiros = atual->proximo;
                    free(atual);
                    atual = listaTiros;
                } else {
                    anterior->proximo = atual->proximo;
                    free(atual);
                    atual = anterior->proximo;
                }
            } else {
                anterior = atual;
                atual = atual->proximo;
            }
        }
    }
}

void desenharCena() {
    int larguraMargem = LARGURA_TELA / 6;
    int larguraRio = LARGURA_TELA - 2 * larguraMargem;

    SDL_Rect rio = {larguraMargem, 0, larguraRio, ALTURA_TELA};
    SDL_SetRenderDrawColor(renderizador, corRio.r, corRio.g, corRio.b, corRio.a);
    SDL_RenderFillRect(renderizador, &rio);

    SDL_Rect margemEsquerda = {0, 0, larguraMargem, ALTURA_TELA};
    SDL_Rect margemDireita = {LARGURA_TELA - larguraMargem, 0, larguraMargem, ALTURA_TELA};
    SDL_SetRenderDrawColor(renderizador, corMato.r, corMato.g, corMato.b, corMato.a);
    SDL_RenderFillRect(renderizador, &margemEsquerda);
    SDL_RenderFillRect(renderizador, &margemDireita);

    int alturaLinha = 10;
    SDL_Rect linhaVerdeEsquerda = {0, ALTURA_TELA - alturaLinha, larguraMargem, alturaLinha};
    SDL_SetRenderDrawColor(renderizador, corMato.r, corMato.g, corMato.b, corMato.a);
    SDL_RenderFillRect(renderizador, &linhaVerdeEsquerda);

    SDL_Rect linhaCiano = {larguraMargem, ALTURA_TELA - alturaLinha, larguraRio, alturaLinha};
    SDL_SetRenderDrawColor(renderizador, corRio.r, corRio.g, corRio.b, corRio.a);
    SDL_RenderFillRect(renderizador, &linhaCiano);

    SDL_Rect linhaVerdeDireita = {LARGURA_TELA - larguraMargem, ALTURA_TELA - alturaLinha, larguraMargem, alturaLinha};
    SDL_SetRenderDrawColor(renderizador, corMato.r, corMato.g, corMato.b, corMato.a);
    SDL_RenderFillRect(renderizador, &linhaVerdeDireita);

    int alturaPonte = 30;
    SDL_Rect ponte = {larguraMargem, ALTURA_TELA - alturaLinha - alturaPonte, larguraRio, alturaPonte};
    SDL_SetRenderDrawColor(renderizador, corPonte.r, corPonte.g, corPonte.b, corPonte.a);
    SDL_RenderFillRect(renderizador, &ponte);

    int larguraCano = 50;
    int alturaCano = 50;
    SDL_Rect retanguloCano = {(LARGURA_TELA - larguraCano) / 2, ALTURA_TELA - alturaLinha - alturaCano - 10, larguraCano, alturaCano};
    SDL_RenderCopy(renderizador, texturaCanhao, NULL, &retanguloCano);
}

void desenharBarraDeVida() {
    int larguraBarra = 200; // Largura inicial da barra
    int alturaBarra = 20;   // Altura da barra
    int xPos = 10;          // Posição X da barra
    int yPos = 10;          // Posição Y da barra

    int larguraAtual = (vidaAtual * larguraBarra) / VIDA_MAXIMA;

    SDL_SetRenderDrawColor(renderizador, 255, 0, 0, 255); // Vermelho
    SDL_Rect barraVida = {xPos, yPos, larguraAtual, alturaBarra};
    SDL_RenderFillRect(renderizador, &barraVida);

    SDL_SetRenderDrawColor(renderizador, 128, 128, 128, 255); // Cinza
    SDL_Rect barraVazia = {xPos + larguraAtual, yPos, larguraBarra - larguraAtual, alturaBarra};
    SDL_RenderFillRect(renderizador, &barraVazia);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (!inicializarSDL()) {
        return -1;
    }

    const int numEmojis = 9;
    SDL_Texture *texturasLixo[numEmojis];
    int probabilidades[] = {23, 23, 12, 10, 10, 10, 5, 5, 2};
    int totalProbabilidades = 0;
    for (int i = 0; i < numEmojis; i++) {
        totalProbabilidades += probabilidades[i];
    }

    if (!carregarMidia(texturasLixo, numEmojis)) {
        SDL_Log("Falha ao carregar mídia!");
        return -1;
    }

    loopMenu(texturasLixo, probabilidades, numEmojis, totalProbabilidades);
    fecharSDL(texturasLixo, numEmojis);
    return 0;
}