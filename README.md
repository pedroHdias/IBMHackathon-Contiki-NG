# IBMHackathon-Contiki-NG
Contiki-NG para o IBM Hackathon

![alt text](https://github.com/pedroHdias/IBMHackathon-Contiki-NG/blob/master/imgs/icon.png)

É utilizado o RaspBerry Pi3 para efetuar a ligação entre a rede WSN e a plataforma IBM Watson IoT. Foi implementado um servidor NodeJS que usa as bibliotecas `dgram` e `udp6` para a comunicação com o Mote, e usa a biblioteca `ibmiotf` para a comunicação com a aplicação na plataforma IBM Watson IoT. Para a rede WSN foram utilizados Zolertias revision-b com o sistema operativo Contiki-NG.

Esquemático da rede:

![alt text](https://github.com/pedroHdias/IBMHackathon-Contiki-NG/blob/master/imgs/esquema_rede.png)

A rede é composta então por 4 componentes distintas:

### IBM Cloud App:

Aplicação alojada na plataforma IBM Watson IoT;

### Servidor NodeJS:

Aplicação que serve dois propósitos. Por um lado contem um servidor UDP que aguarda mensagens periódicas (1-1min) com valores dos sensores (temperatura, humidade e luz) e envia mensagem ao Mote para alterar o estado do atuador (LED). Por outro lado contem um servidor HTTPS que comunica com a aplicação alojada na plataforma IBM Watson IoT, enviando os dados dos sensores e aguardando mensagem para alterar o estado do atuador (LED) do Mote;

### Border Router:

Este dispositivo faz a ligação entre a rede WSN e o meio exterior, encaminhando os pacotes para o Mote e para a interface tun0;

### Mote:

Dispositivo que contem os sensores de temperatura, humidade e luz e envia periodicamente os valores para o servidor NodeJs. Escuta também mensagens provenientes do servidor NodeJs para alterar o estado do seu atuador LED. 

## Usage

O RaspBerry Pi3 corre um script que mantem uma interface virtual tun0 automaticamente ligada. Esta interface virtual é criada através da execução do comando `sudo make connect-router` utilizado o protocolo SLIP (serial line internet protocol), permitindo o encapsulamento e transmissão do tráfego IP para e vindo da linha de série. Para mais detalhes consultar: https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-RPL-border-router. (De notar que o Border Router tem de obrigatoriamente de estar ligado na porta ttyUSB0 e o Mote na porta ttyUSB1. Se isto não se verificar a solução não funciona – para verificar executar o comando `sudo dmesg | grep tty`)

A rede WSN é construída através do uso dos protocolos RPL (Routing over Low Power and Lossy Networks) e ND (Neighbour Discovery), que são implementados sob a pilha protocolar 6LowPan do Contiki-NG. Estes protocolos são utilizados para a descoberta de nós, construção da rede e manutenção da rede. Para mais detalhes consultar: https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-RPL.

Tanto o Border Router como o Mote utilizam o sistema operativo Contiki-NG https://github.com/contiki-ng/contiki-ng. Para o Border Router é utilizado o exemplo `border-router.c`, localizado em `contiki-ng/exemples/rpl-border-router`. Para o Mote é utilizada uma versão adaptada do exemplo `udp-server.c (udp-serverIOT.c)` localizada em `contiki-ng/exemples/rpl-udp`. Para mais detalhes e exemplos, consultar: http://www.iet.unipi.it/c.vallati/files/IoTinfivedays-v1.1.pdf.

## PASSO 1: Para iniciar a rede de sensores é necessário abrir a pasta IBMHackathon-Contiki-NG no Ambiente de Trabalho do Raspberry Pi 3 e efetuar os seguintes comandos:

`cd contiki-ng/examples/rpl-border-router`

`make connect-router`

O comando acima inicializa a interface virtual tun0 e permite a comunicação com a rede WSN – Wireless Sensor Network. De relembrar que este comando só funciona se o Border Router estiver na porta ttyUSB0.

## PASSO 2: Para iniciar o servidor, é necessário estar na directoria do ficheiro, ou seja, em `/IOTServer/`, alterar as configurações para a comunicação com a platarforma IBM Watson IoT (variável `config`) e efetuar o seguinte comando:

`sudo node iotserver.js`

Para a comunicação com a plataforma IBM Watson IoT, os campos “org, id, type, auth-token” diferem para cada grupo.

Exemplo de config:

![alt text](https://github.com/pedroHdias/IBMHackathon-Contiki-NG/blob/master/imgs/exemplo_config.png)

- Nota: Se por algum motivo ocorrer um problema com os módulos de NodeJs, efetuar o seguinte comando na diretoria onde se encontra o ficheiro `iotserver.js`:

`sudo npm i ibmiotf`

Depois destes dois passos a rede terá o seguinte aspeto:

![alt text](https://github.com/pedroHdias/IBMHackathon-Contiki-NG/blob/master/imgs/rede.jpg)

### Se pretender alterar o ficheiro e aplicar as alterações, é necessário parar o serviço do servidor (`Ctrl+C`) no terminal e  efetuar o seguinte comando na diretoria do ficheiro `iotserver.js` para inicializar novamente o servidor Nodejs:

`sudo node iotserver.js`

### Para alteração e upload do ficheiro `udp-serverIOT.c` para o Zolertia é necessário abrir um terminal na diretoria do ficheiro, ou seja, em `contiki-ng/exemples/rpl-udp`, e efetuar os seguintes comandos:

- Fazer upload do código para o Zolertia (MOTE)

`make udp-serverIOT.upload PORT=/dev/ttyUSB1`  

- Se pretender ver logs do Mote na consola, efetuar o seguinte comando

`make PORT=/dev/ttyUSB1 login`

## Importante!!
Se for necessário uma cópia do repositório, encontra-se em https://github.com/pedroHdias/IBMHackathon-Contiki-NG e é feito através do seguinte comando na diretoria pretendida:

`git clone https://github.com/pedroHdias/IBMHackathon-Contiki-NG`

De seguida é necessário entrar na directoria /IBMHachathon-Contiki-NG e efetuar o seguinte comando:

`git clone https://github.com/contiki-ng/contiki-ng`

Depois na directoria /contiki-ng:

`git submodule update --init`

Se necessário, instalar o compilador gcc:

`sudo apt-get install gcc-arm-none-eabi`

Os ficheiros `dht22.h` e `dht22.c` na directoria `/IBMHackathon-Contiki-NG` precisam de ser copiados e substituir os mesmos ficheiros em `/contiki-ng/arch/platform/zoul/dev/`

O ficheiro `udp-serverIOT.c` e `Makefile` precisam de ser copiados para `/contiki-ng/examples/rpl-udp/`

É necessário alterar os seguintes ficheiros: `contiki-ng/os/contiki-default-conf.h` e `IOTServer/iotserver.js`. No ficheiro `contiki-default-conf.h` é necessário alterar a variável:

`#ifndef IEEE802154_CONF_PANID`

`#define IEEE802154_CONF_PANID [DEFINIR PANID DO GRUPO]`

`#endif /* IEEE802154_CONF_PANID */`

No ficheiro `iotserver.js` é necessário alterar as variáveis que diferem de grupo para grupo: 

`var mote = ' ';`

`var config = {`

    "org" : " ",
    
    "id" : " ",
    
    "domain": "internetofthings.ibmcloud.com",
    
    "type" : " ",
    
    "auth-method" : "token",
    
    "auth-token" : " "
    
`};`

# Ambiente Node RED

https://node-red-ipt-session.eu-gb.mybluemix.net
