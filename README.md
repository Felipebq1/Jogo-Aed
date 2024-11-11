Jogo-Aed
Sobre o Jogo
"Jogo-Aed" é um jogo onde você deve proteger o Rio São Francisco de ser poluído por lixos flutuantes. Use seu canhão para atirar e destruir os lixos antes que eles alcancem a margem do rio.

Como Executar o Jogo
Antes de executar o jogo, certifique-se de realizar as seguintes etapas para instalar as dependências necessárias:

Atualize a lista de pacotes:

sudo apt-get update
Instale as bibliotecas SDL necessárias:

sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev
Passos para Compilação e Execução
Clone o repositório ou baixe o código-fonte:

git clone https://github.com/Felipebq1/Jogo-Aed.git
cd Jogo-Aed
Compile o código:

Use o comando abaixo para compilar o programa:

gcc jogo.c -o jogo -lSDL2 -lSDL2_ttf -lSDL2_mixer
Execute o jogo:

Após a compilação bem-sucedida, execute o jogo com:

./jogo
Controles do Jogo
Use o mouse para mirar e clique para atirar nos lixos.
A barra de vida no topo indica a saúde do rio.
Tente manter o Rio São Francisco limpo o maior tempo possível.
