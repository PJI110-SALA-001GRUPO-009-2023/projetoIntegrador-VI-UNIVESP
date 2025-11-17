<h1 align="center"> 🌱 HydraSolo 🌱 </h1>
<h1 align="center">Irrigador automático para hortas urbanas</h1>

![GitHub language count](https://img.shields.io/github/languages/count/PJI110-SALA-001GRUPO-009-2023/projetoIntegrador-VI-UNIVESP?color=%23a2d2ff)
![GitHub repo size](https://img.shields.io/github/repo-size/PJI110-SALA-001GRUPO-009-2023/projetoIntegrador-VI-UNIVESP?color=%23ffc8dd)
![GitHub license](https://img.shields.io/github/license/PJI110-SALA-001GRUPO-009-2023/projetoIntegrador-VI-UNIVESP?color=%23caffbf)

## Descrição
Sistema IoT desenvolvido para automatizar a irrigação e facilitar a gestão comunitária de hortas urbanas, promovendo a agricultura sustentável em ambientes urbanos através de monitoramento inteligente e colaboração entre usuários.

## 🌱 Funcionalidades

### Sistema de Irrigação Inteligente
- Monitoramento em tempo real da umidade do solo
- Controle automático de irrigação baseado em sensores
- Alertas de baixa umidade e necessidade de irrigação
- Histórico de dados ambientais

### Gestão Comunitária
- Cadastro e gerenciamento de hortas urbanas
- Sistema de usuários para gestão colaborativa
- Agendamento de atividades e tarefas
- Compartilhamento de recursos entre membros

## 🏗️ Estrutura do Projeto

```
projetoIntegrador-VI-UNIVESP/
├── LICENSE
├── README.md
├── front-end/                  # Interface web do sistema
│   ├── index.html              # Página de login
│   ├── dashboard.html          # Dashboard principal
│   ├── register-device.html    # Cadastro de dispositivos IoT
│   ├── assets/
│   │   └── img/                # Imagens e recursos visuais
│   ├── css/
│   │   ├── login.css           # Estilos da página de login
│   │   ├── dashboard.css       # Estilos do dashboard
│   │   └── register-device.css # Estilos do cadastro
│   └── js/
│       ├── login.js            # Lógica de autenticação
│       ├── dashboard.js        # Funcionalidades do dashboard
│       └── register-device.js  # Cadastro de dispositivos
|── azure-function/            
|   ├── src/                   
|   |──── functions/         
|   |     └── getLastData.js # Lógica da Azure Function para comunicação entre banco de dados em núvem e front-end
|   └──── index.js/          # configura e hospeda Azure Function
└── device/                     
    ├── config.h                # Configurações de hardware
    ├── iot_configs.h           # Configurações de rede
    ├── SerialLogger.h          # Definição da classe de logs
    ├── SerialLogger.cpp        # Implementação da classe de logs
    ├── calibrador.ino          # Aplicação para captar os parâmetros dos sensores
    ├── AzIoTSasToken.h         # Definição da classe de comunicação com Azure
    ├── AzIoTSasToken.cpp       # Implementação da classe de comunicação com Azure
    └── device.ino              # Aplicação do dispositivo
```

## 🚀 Como usar

### Frontend
1. Abra o arquivo [front-end/index.html](front-end/index.html) em um navegador web
2. Faça login no sistema
3. Acesse o [dashboard](front-end/dashboard.html) para visualizar dados dos sensores
4. Utilize o [cadastro de dispositivos](front-end/register-device.html) para adicionar novos sensores IoT

### Backend
O backend estará disponível em breve com:
- API REST para comunicação com dispositivos IoT
- Banco de dados para armazenamento de dados dos sensores
- Sistema de autenticação e autorização
- Processamento de dados ambientais

### Dispositivo
1. Conecte a mangueira que leva a válvula solenoide a um ponto d'água
2. Coloque o sensor de umidade do solo próximo a área que deseja monitorar
3. Conecte o dispositivo ao Wi-Fi para poder visualizar os dados de umidade do solo e do ar e sua temperatura

## 💻 Tecnologias Utilizadas

### Frontend
- HTML5
- CSS3
- JavaScript (Vanilla)

### Backend (Planejado)
- Node.js / Python (a ser definido)
- Banco de dados (PostgreSQL/MongoDB)
- API REST
- Protocolo MQTT para comunicação IoT

### Hardware IoT
- Sensores de umidade do solo
- Microcontroladores (Arduino/ESP32)
- Válvulas solenoides para irrigação
- Sensores de temperatura e umidade do ar

## 📱 Páginas do Sistema

- **[index.html](front-end/index.html)** - Página de autenticação de usuários
- **[dashboard.html](front-end/dashboard.html)** - Interface principal com dados dos sensores e controles
- **[register-device.html](front-end/register-device.html)** - Cadastro e configuração de dispositivos IoT

## 🎯 Objetivos do Projeto

1. **Sustentabilidade**: Otimizar o uso da água através de irrigação inteligente
2. **Comunidade**: Facilitar a gestão colaborativa de hortas urbanas
3. **Tecnologia**: Aplicar conceitos de IoT na agricultura urbana
4. **Educação**: Promover conhecimento sobre agricultura sustentável

## 🔧 Status do Desenvolvimento

- ✅ Frontend - Interface de usuário implementada
- 🚧 Backend - API em desenvolvimento
- ⏳ Hardware IoT - Integração planejada
- ⏳ Testes de campo - Aguardando conclusão do backend

## 🤝 Contribuição

Este projeto faz parte do Projeto Integrador VI da UNIVESP. Contribuições são bem-vindas seguindo as diretrizes do projeto acadêmico.

## 📄 Licença

Consulte o arquivo [LICENSE](LICENSE) para mais detalhes sobre os termos de uso.

## ⭐ Gostou do Projeto?

Deixe uma estrelinha para nos motivar! ✨

---

**Projeto Integrador VI - UNIVESP**  
