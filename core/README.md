# Core application
An application running on a core server is responsible for receiving data and displaying it within Grafana. This server is powered by MySQL and also features an Adminer instance for added convenience.


# Setup and Usage

### SERVICE.env:
| ENV VARIABLE | Details                   |
| ------------ |-------------------------- |
| CORE_PORT    | Core service port         |
| SINK_PORT    | Sink service port         |
| LMS_PORT     | LMS service port          |
| NUM_DEVICES  | Number of devices in test |
| SAMPLE_FREQ  | Sink sample frequency     |

Create the network
```console
docker network create corenetwork
```

Build the images
```console
./docker.sh SERVICE build
```

Start the server
```console
./docker.sh SERVICE up
```

Stop everything
```console
./docker.sh ALL down
```

List of services:
| SERVICE | Info                             |
| ------- |----------------------------------|
| ALL     | All defined services             |
| BASE    | Bare essentials for baseline     |
| SIP     | The Spanish Inquisition Protocol |
| ASR     | TODO                             |
| LMS     | LMS Filter                       |

Ex. Build and Start core with the Spanish Inquisition Protocol detatched
```console
./docker.sh SIP up -d --build
```


## Exposes

| Port      | Service       |
| --------- | ------------- |
| 123/udp   | NTP           |
| 3000/tcp  | Grafana       |
| 12000/tcp | Core Receiver |
| 12001/tcp | Sink Service  |
| 12002/tcp | Adminer       |
| 12004/tcp | LMS Service   |

## Core
Recieves Utf-8 encoded data, multiple samples can be separated by newline

`DEVICE,ALG,TIMESTAMP,VALUE/n`

| Type      | Format                                        |
| --------- | --------------------------------------------- |
| DEVICE    | Number in range from 0 to "number of devices" |
| ALG       | Alg ID                                        |
| TIMESTAMP | 1695215098.123456                             |
| VALUE     | 13.37                                         |

| ID | Alg Name             |
| -- | -------------------- |
| 1  | Baseline             |
| 2  | Static Filter        |
| 3  | Static Mean Filter   |
| 4  | LMS Filter           |
| 5  | Adaptive Sample Rate |
| 6  | PLA                  |
| 7  | SIP EWMA Filter      |
| 8  | SIP                  |

Ex: `12,2,1695215098.123456,13.37/n`

## Grafana
* Username: admin
* Password: admin

## MySQL / Adminer
* Host:     mysql
* Username: user
* Password: password
* Database: core_db

## SIP Sink
Recieves Utf-8 encoded data, multiple samples can be separated by newline

`DEVICE,TIMESTAMP,K-VALUE,M_VALUE/n`

| Type      | Format                                        |
| --------- | --------------------------------------------- |
| DEVICE    | Number in range from 0 to "number of devices" |
| TIMESTAMP | 1695215098.123456                             |
| K-VALUE   | 13.37                                         |
| M-VALUE   | -3.141                                        |

Ex: `24,1695215098.123456,13.37,-3.141/n`

