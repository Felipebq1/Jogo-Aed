#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>  

#define LARGURA_TELA 800
#define ALTURA_TELA 600
#define LARGURA_LIXO 20
#define ALTURA_LIXO 20
#define VELOCIDADE_LIXO 1
#define LARGURA_TIRO 5
#define ALTURA_TIRO 10
#define VELOCIDADE_TIRO 5
#define DELAY_TIRO 2000  // Delay de 2 segundos em milissegundos

// Definir cores
SDL_Color corRio = {0, 153, 153, 255};  // Cor do rio entre azul e verde (ciano)
SDL_Color corMato = {34, 139, 34, 255}; // Verde para as margens (mato)
SDL_Color corPonte = {169, 169, 169, 255};  // Cor da ponte (cinza)
SDL_Color corLixo = {105, 105, 105, 255};  // Cor do lixo(cinza escuro)
SDL_Color corTiro = {255, 255, 0, 255};  // Cor do tiro(amarelo)

SDL_Window *tela = NULL;
SDL_Renderer *renderizador = NULL;
SDL_Texture *texturaCanhao = NULL;

typedef struct Lixo {
    int x, y;
    struct Lixo *proximo;
} Lixo;

typedef struct Tiro {
    float x, y;
    float vx, vy;
    struct Tiro *proximo;
} Tiro;

// Cabeça da lista ligada para lixos e tiros
Lixo *listaLixos = NULL;
Tiro *listaTiros = NULL;

// Função para adicionar lixo à lista
void adicionarLixos(int x, int y) {
    Lixo *novoLixo = (Lixo *)malloc(sizeof(Lixo));
    novoLixo->x = x;
    novoLixo->y = y;
    novoLixo->proximo = listaLixos;
    listaLixos = novoLixo;
}

// Função para adicionar tiro à lista
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

// Função para atualizar e desenhar lixos
void atualizarEDesenharLixos() {
    Lixo *atual = listaLixos;
    Lixo *anterior = NULL;

    while (atual != NULL) {
        // Atualizar posição
        atual->y += VELOCIDADE_LIXO;

        // Desenhar lixo
        SDL_Rect retanguloLixo = {atual->x, atual->y, LARGURA_LIXO, ALTURA_LIXO};
        SDL_SetRenderDrawColor(renderizador, corLixo.r, corLixo.g, corLixo.b, corLixo.a);
        SDL_RenderFillRect(renderizador, &retanguloLixo);

        // Verificar se o lixo está fora da tela
        if (atual->y > ALTURA_TELA) {
            // Remover lixo da lista
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

// Função para verificar colisão entre dois retângulos
bool verificarColisao(SDL_Rect a, SDL_Rect b) {
    if (a.x + a.w < b.x || a.x > b.x + b.w || a.y + a.h < b.y || a.y > b.y + b.h) {
        return false;
    }
    return true;
}

// Função para atualizar e desenhar tiros
void atualizarEDesenharTiros() {
    Tiro *atual = listaTiros;
    Tiro *anterior = NULL;

    while (atual != NULL) {
        // Atualizar posição
        atual->x += atual->vx;
        atual->y += atual->vy;

        // Criar retângulo para tiro atual
        SDL_Rect retanguloTiro = {(int)atual->x - LARGURA_TIRO / 2, (int)atual->y, LARGURA_TIRO, ALTURA_TIRO};

        // Iterar sobre lixos para colisão
        Lixo *lixoAtual = listaLixos;
        Lixo *lixoAnterior = NULL;
        bool colisaoDetectada = false;

        while (lixoAtual != NULL && !colisaoDetectada) {
            SDL_Rect retanguloLixo = {lixoAtual->x, lixoAtual->y, LARGURA_LIXO, ALTURA_LIXO};

            if (verificarColisao(retanguloTiro, retanguloLixo)) {
                // Remover lixo da lista
                if (lixoAnterior == NULL) {
                    listaLixos = lixoAtual->proximo;
                    free(lixoAtual);
                    lixoAtual = listaLixos;
                } else {
                    lixoAnterior->proximo = lixoAtual->proximo;
                    free(lixoAtual);
                    lixoAtual = lixoAnterior->proximo;
                }
                colisaoDetectada = true; // Sair após colisão
            } else {
                lixoAnterior = lixoAtual;
                lixoAtual = lixoAtual->proximo;
            }
        }

        // Se uma colisão for detectada, remover tiro
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
            // Desenhar tiro
            SDL_SetRenderDrawColor(renderizador, corTiro.r, corTiro.g, corTiro.b, corTiro.a);
            SDL_RenderFillRect(renderizador, &retanguloTiro);

            // Verificar se o tiro está fora da tela
            if (atual->y < 0 || atual->y > ALTURA_TELA || atual->x < 0 || atual->x > LARGURA_TELA) {
                // Remover tiro da lista
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

    // Inicializa SDL_image com suporte a WebP
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_WEBP) & (IMG_INIT_PNG | IMG_INIT_WEBP))) {
        SDL_Log("Erro ao inicializar SDL_image: %s", IMG_GetError());
        return false;
    }
    
    return true;
}

bool carregarMidia() {
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
    return true;
}

void desenharCena() {
    int larguraMargem = LARGURA_TELA / 6;  // Margens menores
    int larguraRio = LARGURA_TELA - 2 * larguraMargem;  // Aumenta o rio

    // Desenhar o rio verticalmente centralizado
    SDL_Rect rio = {larguraMargem, 0, larguraRio, ALTURA_TELA};
    SDL_SetRenderDrawColor(renderizador, corRio.r, corRio.g, corRio.b, corRio.a);
    SDL_RenderFillRect(renderizador, &rio);

    // Desenhar as margens verdes (mato) à esquerda e à direita do rio
    SDL_Rect margemEsquerda = {0, 0, larguraMargem, ALTURA_TELA};
    SDL_Rect margemDireita = {LARGURA_TELA - larguraMargem, 0, larguraMargem, ALTURA_TELA};
    SDL_SetRenderDrawColor(renderizador, corMato.r, corMato.g, corMato.b, corMato.a);
    SDL_RenderFillRect(renderizador, &margemEsquerda);
    SDL_RenderFillRect(renderizador, &margemDireita);

    int alturaLinha = 10;

    // Linha verde à esquerda
    SDL_Rect linhaVerdeEsquerda = {0, ALTURA_TELA - alturaLinha, larguraMargem, alturaLinha};
    SDL_SetRenderDrawColor(renderizador, corMato.r, corMato.g, corMato.b, corMato.a);
    SDL_RenderFillRect(renderizador, &linhaVerdeEsquerda);

    // Linha ciano no centro
    SDL_Rect linhaCiano = {larguraMargem, ALTURA_TELA - alturaLinha, larguraRio, alturaLinha};
    SDL_SetRenderDrawColor(renderizador, corRio.r, corRio.g, corRio.b, corRio.a);
    SDL_RenderFillRect(renderizador, &linhaCiano);

    // Linha verde à direita
    SDL_Rect linhaVerdeDireita = {LARGURA_TELA - larguraMargem, ALTURA_TELA - alturaLinha, larguraMargem, alturaLinha};
    SDL_SetRenderDrawColor(renderizador, corMato.r, corMato.g, corMato.b, corMato.a);
    SDL_RenderFillRect(renderizador, &linhaVerdeDireita);

    // Desenhar a ponte acima da linha de cores
    int alturaPonte = 30;
    SDL_Rect ponte = {larguraMargem, ALTURA_TELA - alturaLinha - alturaPonte, larguraRio, alturaPonte};
    SDL_SetRenderDrawColor(renderizador, corPonte.r, corPonte.g, corPonte.b, corPonte.a);
    SDL_RenderFillRect(renderizador, &ponte);

    // Desenhar a imagem do canhão centralizado na ponte
    int larguraCano = 50;  // Ajuste conforme o tamanho da sua imagem
    int alturaCano = 50; // Ajuste conforme o tamanho da sua imagem
    SDL_Rect retanguloCano = {(LARGURA_TELA - larguraCano) / 2, ALTURA_TELA - alturaLinha - alturaCano - 10, larguraCano, alturaCano};
    SDL_RenderCopy(renderizador, texturaCanhao, NULL, &retanguloCano);
}

void loopJogo() {
    bool rodando = true;
    SDL_Event evento;

    // Determine o centro do canhão
    int canhaoX = LARGURA_TELA / 2;
    int canhaoY = ALTURA_TELA - 50;

    // Controle de tempo para tiros
    Uint32 tempoUltimoTiro = 0;

    while (rodando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            } else if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) {
                // Verifica se o tempo necessário passou para disparar outro tiro
                Uint32 tempoAtual = SDL_GetTicks();
                if (tempoAtual - tempoUltimoTiro >= DELAY_TIRO) {
                    // Pega a posição do mouse para direção do tiro
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    tiros(canhaoX, canhaoY, mouseX, mouseY);
                    tempoUltimoTiro = tempoAtual;  // Atualiza o tempo do último tiro
                }
            }
        }

        // Adicionar lixo aleatório com menor frequência
        if (rand() % 150 == 0) {  // Aumentar o divisor reduz a frequência
            int x = (LARGURA_TELA / 6) + rand() % (LARGURA_TELA * 2 / 3);
            adicionarLixos(x, 0);
        }

        SDL_RenderClear(renderizador);

        // Desenhar a cena
        desenharCena();

        // Atualizar e desenhar lixos
        atualizarEDesenharLixos();
        
        // Atualizar e desenhar tiros
        atualizarEDesenharTiros();

        SDL_RenderPresent(renderizador);
        SDL_Delay(16);  // Controlar FPS
    }
}

void fecharSDL() {
    // Limpar lista de lixos
    while (listaLixos != NULL) {
        Lixo *temp = listaLixos;
        listaLixos = listaLixos->proximo;
        free(temp);
    }

    // Limpar lista de tiros
    while (listaTiros != NULL) {
        Tiro *temp = listaTiros;
        listaTiros = listaTiros->proximo;
        free(temp);
    }

    SDL_DestroyTexture(texturaCanhao);
    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(tela);
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    srand(time(NULL));  // Inicializa o gerador de números aleatórios com a hora atual

    if (!inicializarSDL()) {
        return -1;
    }

    if (!carregarMidia()) {
        SDL_Log("Falha ao carregar mídia!");
        return -1;
    }

    loopJogo();
    fecharSDL();
    return 0;
}