Proyecto Base WebServer IOT

Hardware probado ESP32-WROOM
IDE y Compilador Arduino 2.3.3

El servidor inicia el Modo AP (Access Point) tipicamente en 192.168.4.1 siempre que no se haya seteado SSID y password de la red a utilizar. (1)
Se ingresa en dicha URL para configurar ambos valores. (Los de su red Wi-Fi domestica)
Luego el dispositivo se auto reinicia en el modo Station a la red configurada.
Se puede forzar el Modo AP, presionando el boton de forma prolongada hasta que el led parpadee rapidamente. Luego se ingresa como en 1)
