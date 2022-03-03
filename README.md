# Câmara de Envelhecimento
Repositório de manutenção da câmara de envelhecimento, construído para o Dep. Eng. Civil da Universidade Federal de Viçosa.

## Módulos
A Câmara dispõe de diversos módulos, como:
                
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
                
