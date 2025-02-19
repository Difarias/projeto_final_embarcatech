# Tarefa 4 Unidade 4 - Trabalhando com Interrupções

## Descrição do Projeto

Este projeto tem como objetivo desenvolver um sistema utilizando o microcontrolador RP2040 (Raspberry Pi Pico) e a placa BitDogLab. O sistema é responsável pelo controle de LEDs e botões com interrupções, manipulando uma matriz de LEDs e LEDs RGB de maneira interativa.

O sistema possui a seguinte funcionalidade:
- Um **LED vermelho** do LED RGB pisca 5 vezes por segundo.
- O **Botão A** incrementa o número exibido na matriz de LEDs, e o número ficará **vermelho**.
- O **Botão B** decrementa o número exibido na matriz de LEDs, e o número ficará **azul**.
- **Interrupções** foram utilizadas para o controle dos botões, garantindo resposta imediata às ações do usuário.

## Execução

### Para testar o código, siga um dos métodos abaixo

1. **Usando o Wokwi (Versão VS Code):**
   - Clone o repositório do projeto:
     ```sh
     git clone https://github.com/Difarias/interrupcoes_embarcatechU4_C4
     ```
   - Abra o projeto no **VS Code**.
   - Gere a pasta de build
   - Teste a simulação através da extensão **Raspberry Pi Tools** ou diretamente no **Wokwi integrado ao VScode**.

2. **Usando a placa BitDogLab:**
   - Clone o repositório do projeto.
   - Configure o ambiente de desenvolvimento seguindo as instruções do **Pico SDK**.
   - Abra o projeto no **VS Code**.
   - Importe o projeto através da extensão **Raspberry Pi Tools**.
   - Compile o código, criando a pasta `build` e gerando o binário `.uf2`:
     ```sh
     mkdir build
     cd build
     cmake ..
     make
     ```
   - Copie o arquivo **.uf2** gerado para a placa **BitDogLab** conectada ao computador.

## O que você verá

- O LED vermelho pisca 5 vezes por segundo.
- Ao pressionar o Botão A, o número exibido na matriz de LEDs será incrementado (na cor vermelha).
- Ao pressionar o Botão B, o número exibido na matriz será decrementado (na cor azul).

## Demonstração - Vídeo no YouTube

Para assistir a uma demonstração do projeto no YouTube, acesse o link abaixo:

[![Assista no YouTube](https://img.youtube.com/vi/HMQTizUsbe0/0.jpg)](https://www.youtube.com/shorts/HMQTizUsbe0)


## Agradecimentos

Gostaria de agradecer à equipe do **Embarcatech** por proporcionar esta atividade desafiadora e enriquecedora, uma vez que me agregou bastante no conhecimento. Durante esse processo, pude desenvolver habilidades práticas em sistemas embarcados e na solução de problemas de forma independente.

## Desenvolvedor

- **Diêgo Farias de Freitas**

