# Câmara de Envelhecimento
Repositório de manutenção da câmara de envelhecimento, construído para o Dep. Eng. Civil da Universidade Federal de Viçosa.

## Módulos
A Câmara dispõe de diversos módulos, os quais estão representados nos <a href=“https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/tree/main/Diagramas%20de%20montagem“>diagramas de montagem</a> e estão listados abaixo:
                
+ Arduino MEGA 2560
+ Sensor de Temperatura  DS18B20 (I e II)
+ Sensor de Umidade DHT22
+ Sensor de Irradiância UV ML8511
+ RTC DS3231
+ Módulo de Cartão Micro SD
+ Módulo de Relé (de 1 e 2 canais)
+ Rotary Encoder KY-040
+ Display LCD 16x2
+ Fonte de alimentação MB102
                
----

## Manutenção
Para a maioria dos casos de mau funcionamento da parte eletrônica da câmara, seguem as seguintes instruções:

                
1. Verificar se, ao incializar o dispositivo, alguma mensagem de erro é exibida no display LCD. Caso algum erro seja relatado, siga as orientações descritas na tela.
2. Confira as conexões do módulo que apresenta mau funcionamento, verificando se não há algum cabo solto no interior da caixa, ou se está apenas mal conectado
3. Para alguns casos, existem considerações especiais:
	3.1. No caso de mau funcionamento do relógio (RTC DS3231) confira as conexões caso a data seja "165/165/2165", e confira o estado da bateria (CR2032) caso a data seja algo como "01/01/2000".
	3.2. Caso ocorram erros no arquivamento dos arquivos no cartão de memória, é recomendado retirá-lo e formatá-lo o cartão com o auxílio do <a href=“https://www.sdcard.org/downloads/formatter/“>SD Card Formatter</a>, utilizando a configuração "Overwrite format" (ATENÇÃO, pois isso apagará todos os dados existentes na memória do cartão).
4. Caso nenhum dos procedimentos acima tenha surtido efeito, é recomendado avaliar a integridade do componente que apresenta mau funcionamento, seja a partir de um outro código para o Arduino, ou seja substituindo esse componente por um similar. Todos os componentes podem ser substituídos, sem serem necessárias alterações no código-fonte do Arduino.
                
----

##Conexões
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Diagrama%20de%20Montagem.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Diagrama%20de%20Montagem.png?raw=true)

+ Sensor de Temperatura  DS18B20 (I e II):
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Adaptador%20Cart%C3%A3o%20SD/Cart%C3%A3o%20SD.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Adaptador%20Cart%C3%A3o%20SD/Cart%C3%A3o%20SD.png?raw=true)

+ Sensor de Umidade DHT22:
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/DHT22/DHT22.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/DHT22/DHT22.png?raw=true)

+ Sensor de Irradiância UV ML8511:
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Sensor%20UV%20-%20ML8511/Sensor%20UV%20-%20ML8511.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Sensor%20UV%20-%20ML8511/Sensor%20UV%20-%20ML8511.png?raw=true)

+ RTC DS3231:
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/RTC%20DS3231/RTC%20DS3231.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/RTC%20DS3231/RTC%20DS3231.png?raw=true)

+ Módulo de Cartão Micro SD:
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Adaptador%20Cart%C3%A3o%20SD/Cart%C3%A3o%20SD.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Adaptador%20Cart%C3%A3o%20SD/Cart%C3%A3o%20SD.png?raw=true)

+ Rotary Encoder KY-040:
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Encoder%20KY-040/Endoder%20KY-040.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/Encoder%20KY-040/Endoder%20KY-040.png?raw=true)

+ Display LCD 16x2:
[![](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/DIsplay%20LCD/Display%20LCD.png?raw=true)](https://github.com/AlmirFonseca/CamaraDeEnvelhecimento/blob/main/Diagramas%20de%20montagem/DIsplay%20LCD/Display%20LCD.png?raw=true)

+ Módulo de Relé (de 1 e 2 canais):

