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
#define DELAY_TIRO 900  
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
Uint32 tempoInicioJogo;

// Estruturas de dados para lixos e tiros
typedef struct Lixo {
    int posicaoXLixo, posicaoYLixo;
    int largura, altura;
    SDL_Texture *textura;
    int dano;  // Dano que este tipo de lixo causa
    struct Lixo *proximo;
} Lixo;

typedef struct Tiro {
    float posicaoXTiro, posicaoYTiro;
    float velocidadeXTiro, velocidadeYTiro;
    struct Tiro *proximo;
} Tiro;

// Listas encadeadas para lixos e tiros
Lixo *listaLixos = NULL;
Tiro *listaTiros = NULL;

void loopJogo(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades);
SDL_Texture* carregarEmoji(SDL_Renderer *renderer, TTF_Font *font, const char *emoji);
void adicionarLixos(SDL_Texture *texturas[], int *probabilidades, int numTexturas, int totalProbabilidades);
void tiros(float inicioXTiro, float inicioYTiro, float destinoXTiro, float destinoYTiro);
void atualizarEDesenharLixos();
bool verificarColisao(SDL_Rect a, SDL_Rect b);
void atualizarEDesenharTiros();
bool inicializarSDL();
bool carregarMidia(SDL_Texture **texturasLixo, int numTexturas);
void desenharCena();
void desenharBarraDeVida();
void desenharMenu(Uint32 recorde);
void desenharTempoJogo(Uint32 duracaoJogo);
void loopMenu(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades);
void fecharSDL(SDL_Texture **texturasLixo, int numTexturas);
void salvarPontuacao(Uint32 duracaoJogo);
void desenharMensagemFimDeJogo();
Uint32 lerRecorde();

// Função para desenhar o tempo de jogo atual
// Exibe o tempo decorrido desde o início do jogo no canto inferior esquerdo da tela
void desenharTempoJogo(Uint32 duracaoJogo) {
    SDL_Color branco = {255, 255, 255, 255};
    char texto[50];
    Uint32 minutos = duracaoJogo / 60000; // 60000 ms por minuto
    Uint32 segundos = (duracaoJogo % 60000) / 1000;
    snprintf(texto, sizeof(texto), "Tempo: %02u:%02u", minutos, segundos);

    SDL_Surface *surfaceTexto = TTF_RenderText_Blended(fontePontuacao, texto, branco);
    SDL_Texture *texturaTexto = SDL_CreateTextureFromSurface(renderizador, surfaceTexto);

    SDL_Rect posicaoTexto = {10, ALTURA_TELA - 40, surfaceTexto->w, surfaceTexto->h};
    SDL_RenderCopy(renderizador, texturaTexto, NULL, &posicaoTexto);

    SDL_FreeSurface(surfaceTexto);
    SDL_DestroyTexture(texturaTexto);
}

// Função para salvar a duração do jogo no arquivo e ordenar
// Ordena as pontuações existentes junto com a nova e reescreve o arquivo de pontuações
void salvarPontuacao(Uint32 duracaoJogo) {
    Uint32 pontuacoes[MAX_PONTUACOES];
    int numPontuacoes = 0;

    FILE *arquivo = fopen("pontuacoes.txt", "r");
    if (arquivo) {
        while (fscanf(arquivo, "%u", &pontuacoes[numPontuacoes]) != EOF && numPontuacoes < MAX_PONTUACOES) {
            numPontuacoes++;
        }
        fclose(arquivo);
    }

    if (numPontuacoes < MAX_PONTUACOES) {
        pontuacoes[numPontuacoes++] = duracaoJogo;
    }

    // Ordena as pontuações em ordem decrescente utilizando algoritmo de ordenação por bolha
    for (int indicePontuacao = 0; indicePontuacao < numPontuacoes - 1; indicePontuacao++) {
        for (int indiceComparacao = 0; indiceComparacao < numPontuacoes - indicePontuacao - 1; indiceComparacao++) {
            if (pontuacoes[indiceComparacao] < pontuacoes[indiceComparacao + 1]) {
                Uint32 temp = pontuacoes[indiceComparacao];
                pontuacoes[indiceComparacao] = pontuacoes[indiceComparacao + 1];
                pontuacoes[indiceComparacao + 1] = temp;
            }
        }
    }

    arquivo = fopen("pontuacoes.txt", "w");
    if (arquivo) {
        for (int indicePontuacao = 0; indicePontuacao < numPontuacoes; indicePontuacao++) {
            fprintf(arquivo, "%u\n", pontuacoes[indicePontuacao]);
        }
        fclose(arquivo);
    }
}

// Função para ler o recorde do arquivo
// Retorna a maior pontuação registrada
Uint32 lerRecorde() {
    Uint32 recorde = 0;
    FILE *arquivo = fopen("pontuacoes.txt", "r");
    if (arquivo) {
        fscanf(arquivo, "%u", &recorde);
        fclose(arquivo);
    }
    return recorde;
}

// Função para desenhar o menu inicial com opções de Iniciar e Sair
void desenharMenu(Uint32 recorde) {
    // Defina a cor de fundo para verde escuro
    SDL_SetRenderDrawColor(renderizador, 0, 100, 0, 255);  // Verde escuro
    SDL_RenderClear(renderizador);

    // Carregar a fonte para o título e botões
    TTF_Font *fonteTitulo = TTF_OpenFont("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 64);
    TTF_Font *fonteBotoes = TTF_OpenFont("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 48);
    SDL_Color branco = {255, 255, 255, 255};

    // Renderizar o texto do título
    SDL_Surface *tituloSurface = TTF_RenderUTF8_Blended(fonteTitulo, "Nome do Jogo", branco);
    SDL_Texture *tituloTexto = SDL_CreateTextureFromSurface(renderizador, tituloSurface);

    // Calcular a posição do título para centralizá-lo
    SDL_Rect rectTitulo;
    rectTitulo.x = (LARGURA_TELA - tituloSurface->w) / 2;
    rectTitulo.y = 20;  // Posicionado na parte superior centralizada
    rectTitulo.w = tituloSurface->w;
    rectTitulo.h = tituloSurface->h;
    SDL_RenderCopy(renderizador, tituloTexto, NULL, &rectTitulo);

    // Define cor e posição para o botão "Iniciar"
    SDL_Rect botaoIniciar = {LARGURA_TELA / 2 - 220, ALTURA_TELA / 2 - 25, 200, 50};
    SDL_SetRenderDrawColor(renderizador, 57, 255, 20, 255);  // Verde neon
    SDL_RenderFillRect(renderizador, &botaoIniciar);

    // Renderizar o texto "Iniciar" no botão
    SDL_Surface *textoIniciarSurface = TTF_RenderUTF8_Blended(fonteBotoes, "Iniciar", branco);
    SDL_Texture *textoIniciar = SDL_CreateTextureFromSurface(renderizador, textoIniciarSurface);
    SDL_Rect rectIniciar;
    rectIniciar.x = botaoIniciar.x + (botaoIniciar.w - textoIniciarSurface->w) / 2;
    rectIniciar.y = botaoIniciar.y + (botaoIniciar.h - textoIniciarSurface->h) / 2;
    rectIniciar.w = textoIniciarSurface->w;
    rectIniciar.h = textoIniciarSurface->h;
    SDL_RenderCopy(renderizador, textoIniciar, NULL, &rectIniciar);

    // Define cor e posição para o botão "Sair"
    SDL_Rect botaoSair = {LARGURA_TELA / 2 + 20, ALTURA_TELA / 2 - 25, 200, 50};
    SDL_SetRenderDrawColor(renderizador, 255, 0, 0, 255);  // Vermelho
    SDL_RenderFillRect(renderizador, &botaoSair);

    // Renderizar o texto "Sair" no botão
    SDL_Surface *textoSairSurface = TTF_RenderUTF8_Blended(fonteBotoes, "Sair", branco);
    SDL_Texture *textoSair = SDL_CreateTextureFromSurface(renderizador, textoSairSurface);
    SDL_Rect rectSair;
    rectSair.x = botaoSair.x + (botaoSair.w - textoSairSurface->w) / 2;
    rectSair.y = botaoSair.y + (botaoSair.h - textoSairSurface->h) / 2;
    rectSair.w = textoSairSurface->w;
    rectSair.h = textoSairSurface->h;
    SDL_RenderCopy(renderizador, textoSair, NULL, &rectSair);

    // Renderizar o recorde
    char recordeTexto[50];
    Uint32 minutos = recorde / 60000; // 60000 ms por minuto
    Uint32 segundos = (recorde % 60000) / 1000;
    snprintf(recordeTexto, sizeof(recordeTexto), "Recorde: %02u:%02u", minutos, segundos);

    SDL_Surface *textoRecordeSurface = TTF_RenderText_Blended(fonteBotoes, recordeTexto, branco);
    SDL_Texture *textoRecorde = SDL_CreateTextureFromSurface(renderizador, textoRecordeSurface);

    // Calcular posição do recorde para centralizá-lo na parte inferior
    SDL_Rect rectRecorde;
    rectRecorde.x = (LARGURA_TELA - textoRecordeSurface->w) / 2;
    rectRecorde.y = ALTURA_TELA - 60;  // Deixando um pouco de margem do final da tela
    rectRecorde.w = textoRecordeSurface->w;
    rectRecorde.h = textoRecordeSurface->h;

    SDL_RenderCopy(renderizador, textoRecorde, NULL, &rectRecorde);

    // Apresentar todas as mudanças feitas no renderizador
    SDL_RenderPresent(renderizador);

    // Liberar superfícies e texturas
    SDL_FreeSurface(tituloSurface);
    SDL_DestroyTexture(tituloTexto);
    SDL_FreeSurface(textoIniciarSurface);
    SDL_DestroyTexture(textoIniciar);
    SDL_FreeSurface(textoSairSurface);
    SDL_DestroyTexture(textoSair);
    SDL_FreeSurface(textoRecordeSurface);
    SDL_DestroyTexture(textoRecorde);
    TTF_CloseFont(fonteTitulo);
    TTF_CloseFont(fonteBotoes);
}

// Função para gerenciar o loop do menu inicial
// Lida com eventos de clique do mouse para iniciar o jogo ou sair
void loopMenu(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades) {
    bool rodando = true;
    bool menuAtivo = true;
    SDL_Event evento;

    Uint32 recorde = lerRecorde();

    while (rodando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            } else if (evento.type == SDL_MOUSEBUTTONDOWN && menuAtivo) {
                int posicaoXMouse = evento.button.x;
                int posicaoYMouse = evento.button.y;

                // Definir o retângulo do botão "Iniciar" novamente
                SDL_Rect botaoIniciar = {LARGURA_TELA / 2 - 220, ALTURA_TELA / 2 - 25, 200, 50};
                // Definir o retângulo do botão "Sair" novamente
                SDL_Rect botaoSair = {LARGURA_TELA / 2 + 20, ALTURA_TELA / 2 - 25, 200, 50};

                // Verificar cliques no botão "Iniciar"
                if (posicaoXMouse >= botaoIniciar.x && posicaoXMouse <= botaoIniciar.x + botaoIniciar.w &&
                    posicaoYMouse >= botaoIniciar.y && posicaoYMouse <= botaoIniciar.y + botaoIniciar.h) {
                    // Iniciar Jogo
                    menuAtivo = false;
                    loopJogo(texturasLixo, probabilidades, numTexturas, totalProbabilidades);
                } 
                // Verificar cliques no botão "Sair"
                else if (posicaoXMouse >= botaoSair.x && posicaoXMouse <= botaoSair.x + botaoSair.w &&
                         posicaoYMouse >= botaoSair.y && posicaoYMouse <= botaoSair.y + botaoSair.h) {
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

// Função principal do loop do jogo
// Controla a lógica de tempo de jogo, eventos, e atualizações da tela
void loopJogo(SDL_Texture **texturasLixo, int *probabilidades, int numTexturas, int totalProbabilidades) {
    bool rodando = true;
    SDL_Event evento;

    int posicaoXCanhao = LARGURA_TELA / 2;
    int posicaoYCanhao = ALTURA_TELA - 50;
    Uint32 tempoUltimoTiro = 0;  // Variável para controlar o tempo do último tiro
    tempoInicioJogo = SDL_GetTicks(); // Captura o tempo de início do jogo

    while (rodando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            } else if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) {
                Uint32 tempoAtual = SDL_GetTicks();
                if (tempoAtual - tempoUltimoTiro >= DELAY_TIRO) {
                    int posicaoXMouse, posicaoYMouse;
                    SDL_GetMouseState(&posicaoXMouse, &posicaoYMouse);
                    tiros(posicaoXCanhao, posicaoYCanhao, posicaoXMouse, posicaoYMouse);
                    tempoUltimoTiro = tempoAtual;
                }
            }
        }

        if (vidaAtual <= 0) {
            // Se a vida acabar, mostrar a mensagem de fim de jogo
            desenharCena();
            desenharMensagemFimDeJogo();
            SDL_RenderPresent(renderizador);
            SDL_Delay(3000); // Delay para visualizar a mensagem de fim de jogo por 3 segundos
            Uint32 tempoTerminoJogo = SDL_GetTicks(); // Tempo quando o jogo termina
            Uint32 duracaoJogo = tempoTerminoJogo - tempoInicioJogo; // Calcula a duração do jogo em milissegundos
            salvarPontuacao(duracaoJogo); // Salva a duração do jogo
            rodando = false;
            continue;
        }

        if (rand() % 50 == 0) {
            adicionarLixos(texturasLixo, probabilidades, numTexturas, totalProbabilidades);
        }

        SDL_RenderClear(renderizador);

        desenharCena();
        desenharBarraDeVida();
        atualizarEDesenharLixos();
        atualizarEDesenharTiros();
        desenharTempoJogo(SDL_GetTicks() - tempoInicioJogo); // Desenhar o tempo de jogo atual

        SDL_RenderPresent(renderizador);
        SDL_Delay(16);
    }
}

// Inicializa todos os componentes SDL necessários
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

// Carrega todas as texturas e fontes necessárias para o jogo
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
    for (int indiceEmoji = 0; indiceEmoji < numTexturas; indiceEmoji++) {
        texturasLixo[indiceEmoji] = carregarEmoji(renderizador, font, emojis[indiceEmoji]);
        if (!texturasLixo[indiceEmoji]) {
            SDL_Log("Erro ao criar textura do emoji de lixo: %s", SDL_GetError());
            TTF_CloseFont(font);
            return false;
        }
    }

    // Inicializa a fonte para pontuação
    fontePontuacao = TTF_OpenFont("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 15);
    if (!fontePontuacao) {
        SDL_Log("Erro ao carregar fonte de pontuação: %s", TTF_GetError());
        return false;
    }

    TTF_CloseFont(font);
    return true;
}

// Libera recursos e fecha o SDL de forma segura
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

    for (int indiceTextura = 0; indiceTextura < numTexturas; indiceTextura++) {
        SDL_DestroyTexture(texturasLixo[indiceTextura]);
    }

    TTF_CloseFont(fontePontuacao);
    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(tela);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

// Carrega um emoji como textura para ser usado no jogo
SDL_Texture* carregarEmoji(SDL_Renderer *renderer, TTF_Font *font, const char *emoji) {
    SDL_Color branco = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, emoji, branco);
    SDL_Texture *textura = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return textura;
}

// Adiciona novos lixos na lista de lixos existentes, com base em probabilidades
void adicionarLixos(SDL_Texture *texturas[], int *probabilidades, int numTexturas, int totalProbabilidades) {
    int larguraMargem = LARGURA_TELA / 6;
    int larguraRio = LARGURA_TELA - 2 * larguraMargem;

    // Gera a posição X do lixo para estar apenas dentro do rio
    int posicaoXLixo = larguraMargem + rand() % (larguraRio - LARGURA_LIXO);

    int sorteio = rand() % totalProbabilidades;
    int acumuladorProbabilidades = 0;
    int indiceLixo = 0;
    for (int indiceProbabilidade = 0; indiceProbabilidade < numTexturas; indiceProbabilidade++) {
        acumuladorProbabilidades += probabilidades[indiceProbabilidade];
        if (sorteio < acumuladorProbabilidades) {
            indiceLixo = indiceProbabilidade;
            break;
        }
    }

    Lixo *novoLixo = (Lixo *)malloc(sizeof(Lixo));
    novoLixo->posicaoXLixo = posicaoXLixo;
    novoLixo->posicaoYLixo = 0;
    novoLixo->textura = texturas[indiceLixo];

    // Ajusta o tamanho do lixo com base no tipo
    if (indiceLixo == 2 || indiceLixo == 6 || indiceLixo == 7 || indiceLixo == 8) {  // 📦, 🛞, 🪑, 🚽
        novoLixo->largura = LARGURA_LIXO * 2;
        novoLixo->altura = ALTURA_LIXO * 2;
    } else if(indiceLixo == 4 || indiceLixo == 5) { // 👕, 🎒
        novoLixo->largura = LARGURA_LIXO * 1.5;
        novoLixo->altura = ALTURA_LIXO * 1.5;
    } else if(indiceLixo == 3) { // 👟
        novoLixo->largura = LARGURA_LIXO * 1.2;
        novoLixo->altura = ALTURA_LIXO * 1.2;
    } else {   // 🥤, 🧃
        novoLixo->largura = LARGURA_LIXO;
        novoLixo->altura = ALTURA_LIXO;
    }

    // Define o dano com base no tipo de lixo
    if (indiceLixo == 0 || indiceLixo == 1) {  // 🥤, 🧃
        novoLixo->dano = DANO_BAIXO;
    } else if (indiceLixo == 2 || indiceLixo == 3 || indiceLixo == 4 || indiceLixo == 5) {  // 📦, 👟, 👕, 🎒
        novoLixo->dano = DANO_MEDIO;
    } else if (indiceLixo == 6 || indiceLixo == 7) {  // 🛞, 🪑
        novoLixo->dano = DANO_ALTO;
    } else if (indiceLixo == 8) {  // 🚽
        novoLixo->dano = DANO_MUITO_ALTO;
    }

    novoLixo->proximo = listaLixos;
    listaLixos = novoLixo;
}

// Cria e adiciona um novo tiro à lista de tiros
void tiros(float inicioXTiro, float inicioYTiro, float destinoXTiro, float destinoYTiro) {
    Tiro *novoTiro = (Tiro *)malloc(sizeof(Tiro));
    novoTiro->posicaoXTiro = inicioXTiro;
    novoTiro->posicaoYTiro = inicioYTiro;

    // Calcular direção do tiro
    float diferencaX = destinoXTiro - inicioXTiro;
    float diferencaY = destinoYTiro - inicioYTiro;
    float comprimento = sqrtf(diferencaX * diferencaX + diferencaY * diferencaY);
    if (comprimento != 0) {
        novoTiro->velocidadeXTiro = VELOCIDADE_TIRO * (diferencaX / comprimento);
        novoTiro->velocidadeYTiro = VELOCIDADE_TIRO * (diferencaY / comprimento);
    } else {
        novoTiro->velocidadeXTiro = 0;
        novoTiro->velocidadeYTiro = -VELOCIDADE_TIRO; // Padrão para cima se não houver movimento
    }

    novoTiro->proximo = listaTiros;
    listaTiros = novoTiro;
}

// Atualiza a posição dos lixos e os desenha na tela
// Remove lixos que saem da tela e atualiza a vida do rio
void atualizarEDesenharLixos() {
    Lixo *lixoAtual = listaLixos;
    Lixo *lixoAnterior = NULL;

    while (lixoAtual != NULL) {
        lixoAtual->posicaoYLixo += VELOCIDADE_LIXO;

        SDL_Rect retanguloLixo = {lixoAtual->posicaoXLixo, lixoAtual->posicaoYLixo, lixoAtual->largura, lixoAtual->altura};
        SDL_RenderCopy(renderizador, lixoAtual->textura, NULL, &retanguloLixo);

        if (lixoAtual->posicaoYLixo > ALTURA_TELA) {
            // Reduz a vida de acordo com o dano do lixo
            vidaAtual -= lixoAtual->dano;
            if (vidaAtual < 0) {
                vidaAtual = 0;
            }

            // Remove o lixo da lista
            if (lixoAnterior == NULL) {
                listaLixos = lixoAtual->proximo;
                free(lixoAtual);
                lixoAtual = listaLixos;
            } else {
                lixoAnterior->proximo = lixoAtual->proximo;
                free(lixoAtual);
                lixoAtual = lixoAnterior->proximo;
            }
        } else {
            lixoAnterior = lixoAtual;
            lixoAtual = lixoAtual->proximo;
        }
    }
}

// Verifica se dois retângulos colidem
bool verificarColisao(SDL_Rect retanguloA, SDL_Rect retanguloB) {
    return !(retanguloA.x + retanguloA.w < retanguloB.x || retanguloA.x > retanguloB.x + retanguloB.w || retanguloA.y + retanguloA.h < retanguloB.y || retanguloA.y > retanguloB.y + retanguloB.h);
}

// Atualiza a posição dos tiros e verifica colisões com lixos
// Remove tiros e lixos que colidem ou saem da tela
void atualizarEDesenharTiros() {
    Tiro *tiroAtual = listaTiros;
    Tiro *tiroAnterior = NULL;

    while (tiroAtual != NULL) {
        tiroAtual->posicaoXTiro += tiroAtual->velocidadeXTiro;
        tiroAtual->posicaoYTiro += tiroAtual->velocidadeYTiro;

        SDL_Rect retanguloTiro = {(int)tiroAtual->posicaoXTiro - 10 / 2, (int)tiroAtual->posicaoYTiro, 20, 20};

        Lixo *lixoAtual = listaLixos;
        Lixo *lixoAnterior = NULL;
        bool colisaoDetectada = false;

        while (lixoAtual != NULL && !colisaoDetectada) {
            SDL_Rect retanguloLixo = {lixoAtual->posicaoXLixo, lixoAtual->posicaoYLixo, lixoAtual->largura, lixoAtual->altura};

            if (verificarColisao(retanguloTiro, retanguloLixo)) {
                // Remove o lixo que colidiu
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
            // Remove o tiro da lista se houve colisão
            if (tiroAnterior == NULL) {
                listaTiros = tiroAtual->proximo;
                free(tiroAtual);
                tiroAtual = listaTiros;
            } else {
                tiroAnterior->proximo = tiroAtual->proximo;
                free(tiroAtual);
                tiroAtual = tiroAnterior->proximo;
            }
        } else {
            SDL_RenderCopy(renderizador, texturaTiro, NULL, &retanguloTiro);

            // Remove o tiro se saiu da tela
            if (tiroAtual->posicaoYTiro < 0 || tiroAtual->posicaoYTiro > ALTURA_TELA || tiroAtual->posicaoXTiro < 0 || tiroAtual->posicaoXTiro > LARGURA_TELA) {
                if (tiroAnterior == NULL) {
                    listaTiros = tiroAtual->proximo;
                    free(tiroAtual);
                    tiroAtual = listaTiros;
                } else {
                    tiroAnterior->proximo = tiroAtual->proximo;
                    free(tiroAtual);
                    tiroAtual = tiroAnterior->proximo;
                }
            } else {
                tiroAnterior = tiroAtual;
                tiroAtual = tiroAtual->proximo;
            }
        }
    }
}

// Desenha o cenário do jogo, incluindo o rio e as margens
void desenharCena() {
    int larguraMargem = LARGURA_TELA / 6;
    int larguraRio = LARGURA_TELA - 2 * larguraMargem;

    // Desenhar o rio
    SDL_Rect rio = {larguraMargem, 0, larguraRio, ALTURA_TELA};
    SDL_SetRenderDrawColor(renderizador, corRio.r, corRio.g, corRio.b, corRio.a);
    SDL_RenderFillRect(renderizador, &rio);

    // Desenhar as margens verdes
    SDL_Rect margemEsquerda = {0, 0, larguraMargem, ALTURA_TELA};
    SDL_Rect margemDireita = {LARGURA_TELA - larguraMargem, 0, larguraMargem, ALTURA_TELA};
    SDL_SetRenderDrawColor(renderizador, corMato.r, corMato.g, corMato.b, corMato.a);
    SDL_RenderFillRect(renderizador, &margemEsquerda);
    SDL_RenderFillRect(renderizador, &margemDireita);

    TTF_Font *fontArvores = TTF_OpenFont("/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf", 36);

    // Desenhar fileiras de árvores na margem esquerda
    SDL_Texture *texturaArvore = carregarEmoji(renderizador, fontArvores, "🌳");
    int esquerdaArvoresPosicoesY[] = {60, 130, 190, 250, 310, 370, 430, 490}; 
    for (int indiceArvore = 0; indiceArvore < 8; indiceArvore++) {  
        SDL_Rect posicaoArvore1 = {10, esquerdaArvoresPosicoesY[indiceArvore], 40, 40};
        SDL_Rect posicaoArvore2 = {60, esquerdaArvoresPosicoesY[indiceArvore], 40, 40};
        SDL_RenderCopy(renderizador, texturaArvore, NULL, &posicaoArvore1);
        SDL_RenderCopy(renderizador, texturaArvore, NULL, &posicaoArvore2);
    }

    // Desenhar prédios e árvores alternados no topo da margem direita
    SDL_Texture *texturaPredio = carregarEmoji(renderizador, fontArvores, "🏢");
    int direitaElementosPosicoesY[] = {10, 70};

    for (int indiceElemento = 0; indiceElemento < 2; indiceElemento++) {
        if (indiceElemento % 2 == 0) {
            SDL_Rect posicaoPredio = {LARGURA_TELA - larguraMargem + 10, direitaElementosPosicoesY[indiceElemento], 60, 60};
            SDL_Rect posicaoArvore = {LARGURA_TELA - larguraMargem + 80, direitaElementosPosicoesY[indiceElemento], 40, 40};
            SDL_RenderCopy(renderizador, texturaPredio, NULL, &posicaoPredio);
            SDL_RenderCopy(renderizador, texturaArvore, NULL, &posicaoArvore);
        } else {
            SDL_Rect posicaoArvore = {LARGURA_TELA - larguraMargem + 10, direitaElementosPosicoesY[indiceElemento], 40, 40};
            SDL_Rect posicaoPredio = {LARGURA_TELA - larguraMargem + 80, direitaElementosPosicoesY[indiceElemento], 60, 60};
            SDL_RenderCopy(renderizador, texturaArvore, NULL, &posicaoArvore);
            SDL_RenderCopy(renderizador, texturaPredio, NULL, &posicaoPredio);
        }
    }

    // Desenhar duas fileiras de árvores no restante da margem direita
    for (int indiceArvore = 2; indiceArvore < 10; indiceArvore++) {
        SDL_Rect posicaoArvore1 = {LARGURA_TELA - larguraMargem + 10, direitaElementosPosicoesY[indiceArvore % 2] + 130 * (indiceArvore / 2), 40, 40};
        SDL_Rect posicaoArvore2 = {LARGURA_TELA - larguraMargem + 80, direitaElementosPosicoesY[indiceArvore % 2] + 130 * (indiceArvore / 2), 40, 40};
        SDL_RenderCopy(renderizador, texturaArvore, NULL, &posicaoArvore1);
        SDL_RenderCopy(renderizador, texturaArvore, NULL, &posicaoArvore2);
    }

    // Destruir texturas de emojis após uso
    SDL_DestroyTexture(texturaArvore);
    SDL_DestroyTexture(texturaPredio);
    TTF_CloseFont(fontArvores);

    // Linha verde e ciano no final
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

    // Desenhar a ponte
    int alturaPonte = 30;
    SDL_Rect ponte = {larguraMargem, ALTURA_TELA - alturaLinha - alturaPonte, larguraRio, alturaPonte};
    SDL_SetRenderDrawColor(renderizador, corPonte.r, corPonte.g, corPonte.b, corPonte.a);
    SDL_RenderFillRect(renderizador, &ponte);

    // Desenhar o canhão
    int larguraCano = 50;
    int alturaCano = 50;
    SDL_Rect retanguloCano = {(LARGURA_TELA - larguraCano) / 2, ALTURA_TELA - alturaLinha - alturaCano - 10, larguraCano, alturaCano};
    SDL_RenderCopy(renderizador, texturaCanhao, NULL, &retanguloCano);
}

// Desenha a barra de vida no topo da tela
void desenharBarraDeVida() {
    int larguraBarra = 200; // Largura inicial da barra
    int alturaBarra = 20;   // Altura da barra
    int posicaoXBarra = 10; // Posição X da barra
    int posicaoYBarra = 10; // Posição Y da barra

    int larguraAtual = (vidaAtual * larguraBarra) / VIDA_MAXIMA;

    SDL_SetRenderDrawColor(renderizador, 255, 0, 0, 255); // Vermelho
    SDL_Rect barraVida = {posicaoXBarra, posicaoYBarra, larguraAtual, alturaBarra};
    SDL_RenderFillRect(renderizador, &barraVida);

    SDL_SetRenderDrawColor(renderizador, 128, 128, 128, 255); // Cinza
    SDL_Rect barraVazia = {posicaoXBarra + larguraAtual, posicaoYBarra, larguraBarra - larguraAtual, alturaBarra};
    SDL_RenderFillRect(renderizador, &barraVazia);
}

// Mostra a mensagem de fim de jogo no centro da tela
void desenharMensagemFimDeJogo() {
    // Definir a mensagem de fim de jogo
    const char* mensagem = "Fim de jogo! O mar foi poluído demais.";
    SDL_Color branco = {255, 255, 255, 255}; // Cor branca para o texto

    // Renderizar o texto da mensagem
    SDL_Surface *surfaceMensagem = TTF_RenderUTF8_Blended(fontePontuacao, mensagem, branco);
    SDL_Texture *texturaMensagem = SDL_CreateTextureFromSurface(renderizador, surfaceMensagem);

    // Calcular a posição para centralizar a mensagem
    SDL_Rect posicaoMensagem;
    posicaoMensagem.x = (LARGURA_TELA - surfaceMensagem->w) / 2;
    posicaoMensagem.y = (ALTURA_TELA - surfaceMensagem->h) / 2;
    posicaoMensagem.w = surfaceMensagem->w;
    posicaoMensagem.h = surfaceMensagem->h;

    SDL_RenderCopy(renderizador, texturaMensagem, NULL, &posicaoMensagem);

    // Liberar recursos utilizados
    SDL_FreeSurface(surfaceMensagem);
    SDL_DestroyTexture(texturaMensagem);
}

// Função principal
int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (!inicializarSDL()) {
        return -1;
    }

    const int numEmojis = 9;
    SDL_Texture *texturasLixo[numEmojis];
    int probabilidades[] = {23, 23, 12, 10, 10, 10, 5, 5, 2};
    int totalProbabilidades = 0;
    for (int indiceEmoji = 0; indiceEmoji < numEmojis; indiceEmoji++) {
        totalProbabilidades += probabilidades[indiceEmoji];
    }

    if (!carregarMidia(texturasLixo, numEmojis)) {
        SDL_Log("Falha ao carregar mídia!");
        return -1;
    }

    loopMenu(texturasLixo, probabilidades, numEmojis, totalProbabilidades);
    fecharSDL(texturasLixo, numEmojis);
    return 0;
}