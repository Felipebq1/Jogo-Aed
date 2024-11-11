# Jogo-Aed

# Como Executar o Jogo
Antes de executar o jogo, certifique-se de realizar as seguintes etapas para instalar as dependências necessárias:

# 1. Atualize a lista de pacotes:
   sudo apt-get update
# 2. Instale as bibliotecas SDL necessárias:
   sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev

# Passos para Compilação e Execução
# 1. Clone o repositório ou baixe o código-fonte:
   git clone [https://github.com/Felipebq1/Jogo-Aed.git]
   cd [cd Jogo-Aed]

# 2. Compile o código:
Use o comando abaixo para compilar o programa:
   gcc jogo.c -o jogo -lSDL2 -lSDL2_ttf -lSDL2_mixer

# 3. Execute o jogo:
Após a compilação bem-sucedida, execute o jogo com:
   ./jogo
