# Core application
An application running on a core server is responsible for receiving data and displaying it within Grafana. This server is powered by MySQL and also features an Adminer instance for added convenience.


# Setup and Usage

Create the network
```console
docker network create corenetwork
```

Build the images
```console
docker-compose build
```

Start the server
```console
docker-compose up
```

## Exposes

| Port      | Service       |
| --------- | ------------- |
| 3000/tcp  | Grafana       |
| 12000/tcp | Core Receiver |
| 12002/tcp | Adminer       |

## Core
Recieves Utf-8 encoded data, multiple samples can be separated by newline

`ALG,TIMESTAMP,VALUE/n`

| Type      | Format             |
| --------- | ------------------ |
| ALG       | {1, 2, 3, 4, 5, 6} |
| TIMESTAMP | 1695215098.123456  |
| VALUE     | 13.37              |

Ex: `1,1695215098.123456,13,37/n`

## Grafana
* Username: admin
* Password: admin

## MySQL / Adminer
* Host:     mysql
* Username: user
* Password: password
* Database: core_db

