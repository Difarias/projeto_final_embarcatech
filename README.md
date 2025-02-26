# Projeto Final - Embarcatech

## Descrição do Projeto

Nos dias de hoje, passamos longas horas conectados, seja estudando, trabalhando ou realizando atividades de lazer. Esse tempo prolongado diante das telas pode causar fadiga ocular, problemas posturais e até mesmo impactar a produtividade. Pensando nisso, desenvolvi este sistema embarcado para atuar como um assistente de bem-estar digital.

O projeto funciona como um lembrete interativo, alertando o usuário para realizar pausas regulares, testar seus reflexos e realizar alongamentos, promovendo hábitos mais saudáveis durante o uso prolongado do computador.

A funcionalidade principal do sistema é um temporizador configurável que, ao fim de cada período definido, dispara um alarme sonoro e conduz o usuário por um ciclo de descanso e atividades simples para relaxamento.

### Fluxograma do Sistema
1. Inicialização do sistema e configuração dos pinos.
2. O usuário pressiona o botão A para definir o tempo do alarme.
3. Após definir o tempo, o usuário pressiona o botão B para salvar o tempo.
4. O temporizador inicia a contagem e dispara o alarme (buzzer) ao final.
5. O usuário pressiona B para pausar o alarme e iniciar o teste de reflexo.
6. Após o teste de reflexo, o sistema aguarda 20 segundos para descanso dos olhos.
7. O sistema inicia o teste de alongamento guiado.
8. O ciclo se repete.

## Requisitos

Antes de começar, você precisará de alguns itens e ferramentas:

- **Placa Raspberry Pi Pico** (BitDogLab no caso do Embarcatech)
- **Cabo micro-USB** para conexão à placa
- **Joystick analógico** para controle
- **LEDs** para teste e para Matriz
- **Display OLED** (por exemplo, baseado no SSD1306)
- **Buzzer**
- **Ferramentas de desenvolvimento**:
  - `git` (para clonar o repositório)
  - `CMake` (para gerar os arquivos de compilação)
  - `Make` (para compilar o código)
  - `SDK do Raspberry Pi Pico` (Raspberry Pi Pico SDK)

## Execução

### Para testar o código, siga o método abaixo:

1. Passo a Passo:
   - 1: Clone o repositório do projeto e entre na pasta:
     ```sh
     git clone https://github.com/Difarias/projeto_final_embarcatech/
     cd projeto_final_embarcatech/
     ```
   - 2: Abra o projeto no **VS Code**.
     ```sh
     code . 
     ```
   - 3: Primeiro, crie um diretório para a compilação do projeto:
      ```bash
      mkdir build
      cd build
      ```
   - 4: Configurar o CMake
     Dentro do diretório `build`, configure o projeto com o CMake:
      ```bash
      cmake ..
      ```
   - 5: Compilar o código
     Agora, compile o projeto com o `make`:
      ```bash
      make
      ```
      Este comando irá gerar os arquivos de compilação necessários.
   - 6: Carregar o código na Placa
      Após a compilação, você precisará carregar o código na sua placa Raspberry Pi Pico. Para isso, pressione o botão **BOOTSEL** na placa enquanto a conecta ao    computador via USB. Isso fará com que a placa seja reconhecida como um dispositivo de armazenamento em massa.

Copie o arquivo `.uf2` gerado pelo comando `make` para a memória da placa Raspberry Pi Pico. Após copiar o arquivo, a placa será reiniciada automaticamente e começará a executar o código.

## Demonstração - Vídeo no YouTube

Para assistir a uma demonstração do projeto no YouTube, acesse o link abaixo:

[![Assista no YouTube](https://img.youtube.com/vi/OS9fO3CU-O8/0.jpg)](https://youtu.be/OS9fO3CU-O8)

## Agradecimentos

Esta jornada foi extremamente interessante e desafiadora, proporcionando um aprendizado valioso em sistemas embarcados. Durante o desenvolvimento deste projeto, pude aprimorar minhas habilidades técnicas e fortalecer minha capacidade de solucionar problemas de forma autônoma.

Agradeço imensamente à equipe do Embarcatech por proporcionar essa experiência enriquecedora. Um agradecimento especial ao mentor Giovanni Antherrely, que sempre esteve presente para esclarecer dúvidas e oferecer suporte fundamental ao longo do projeto. Também sou grato aos professores Wilton Lacerda e Jorge Wattes, cujo conhecimento e orientação foram essenciais para aprofundar minha compreensão dos conteúdos abordados.

Sinto-me agora mais preparado para enfrentar novos desafios, seja em futuras iniciativas com o próprio CEPEDI ou em projetos individuais. A experiência adquirida aqui será fundamental para minha trajetória, e sou grato pela oportunidade de expandir meus conhecimentos nesse campo fascinante.

## Desenvolvedor

- **Diêgo Farias de Freitas**

