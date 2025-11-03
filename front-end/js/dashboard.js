document.addEventListener('DOMContentLoaded', () => {
    const gardenStatusElement = document.getElementById('gardenStatus');
    const lastWateringTimeElement = document.getElementById('lastWateringTime');
    const soilHumidityElement = document.getElementById('soilHumidity');
    const temperatureElement = document.getElementById('temperature');
    const airHumidityElement = document.getElementById('airHumidity');
    const lastReadingTimeElement = document.getElementById('lastReadingTime');
    const manualWateringBtn = document.getElementById('manualWateringBtn');
    const profileLink = document.getElementById('profileLink');
    const logoutLink = document.getElementById('logoutLink');

    function checkAuthentication() {
        const authToken = localStorage.getItem('authToken');
        if (!authToken) {
            window.location.href = 'index.html';
        }
        return authToken;
    }

    const authToken = checkAuthentication();

    async function fetchDashboardData() {
        if (!authToken) return;

        // SIMULAÇÃO DE DADOS DO DASHBOARD:
        // Em um ambiente real, esta parte faria uma requisição à sua API.
        const mockDashboardData = {
            generalStatus: 'Sua horta está saudável e produtiva!',
            soilHumidity: 72,
            temperature: 26,
            airHumidity: 85,
            lastWatering: new Date(new Date().setHours(new Date().getHours() - 3, 0, 0)).toISOString(), 
            lastReading: new Date().toISOString(), // Agora
        };

        // Simula um atraso de rede para parecer uma requisição real
        await new Promise(resolve => setTimeout(resolve, 500));

        updateDashboardUI(mockDashboardData);

        // A parte original com `fetch` (comentada para simulação):
        /*
        const DASHBOARD_API_URL = 'SUA_URL_DA_API_DO_DASHBOARD';

        try {
            const response = await fetch(DASHBOARD_API_URL, {
                method: 'GET',
                headers: {
                    'Authorization': `Bearer ${authToken}`,
                    'Content-Type': 'application/json',
                },
            });

            if (response.ok) {
                const data = await response.json();
                updateDashboardUI(data);
            } else {
                if (response.status === 401 || response.status === 403) {
                    alert('Sessão expirada ou não autorizada. Faça login novamente.');
                    localStorage.removeItem('authToken');
                    window.location.href = 'index.html';
                } else {
                    alert('Não foi possível carregar os dados do dashboard.');
                }
            }
        } catch (error) {
            alert('Erro de conexão ao carregar o dashboard.');
        }
        */
    }

    function updateDashboardUI(data) {
        gardenStatusElement.textContent = data.generalStatus || 'Carregando...';
        soilHumidityElement.textContent = `${data.soilHumidity || 0}%`;
        temperatureElement.textContent = `${data.temperature || 0}°C`;
        airHumidityElement.textContent = `${data.airHumidity || 0}%`;

        const formatDateTime = (isoString) => {
            if (!isoString) return 'N/A';
            const date = new Date(isoString);
            const today = new Date();
            const isToday = date.toDateString() === today.toDateString();
            const time = date.toLocaleTimeString('pt-BR', { hour: '2-digit', minute: '2-digit' });
            return isToday ? `Hoje, ${time}` : `${date.toLocaleDateString('pt-BR')} ${time}`;
        };

        lastWateringTimeElement.textContent = formatDateTime(data.lastWatering);
        lastReadingTimeElement.textContent = formatDateTime(data.lastReading);
    }

    if (manualWateringBtn) {
        manualWateringBtn.addEventListener('click', async () => {
            if (!authToken) return;

            // SIMULAÇÃO DE REGA MANUAL:
            // Em um ambiente real, esta parte faria uma requisição à sua API.
            alert('Comando de rega manual enviado com sucesso (simulado)!');
            fetchDashboardData(); // Recarrega os dados para simular a atualização
            
            // A parte original com `fetch` (comentada para simulação):
            /*
            const MANUAL_WATERING_API_URL = 'SUA_URL_DA_API_DE_REGA_MANUAL';

            if (confirm('Tem certeza que deseja iniciar uma rega manual?')) {
                try {
                    const response = await fetch(MANUAL_WATERING_API_URL, {
                        method: 'POST',
                        headers: {
                            'Authorization': `Bearer ${authToken}`,
                            'Content-Type': 'application/json',
                        },
                    });

                    if (response.ok) {
                        alert('Comando de rega manual enviado com sucesso!');
                        fetchDashboardData();
                    } else {
                        const errorData = await response.json();
                        alert('Erro ao iniciar rega manual: ' + (errorData.message || 'Erro desconhecido.'));
                    }
                } catch (error) {
                    alert('Erro de conexão ao tentar regar manualmente.');
                }
            }
            */
        });
    }

    if (profileLink) {
        profileLink.addEventListener('click', (event) => {
            event.preventDefault();
            alert('Funcionalidade de perfil em desenvolvimento!');
        });
    }

    if (logoutLink) {
        logoutLink.addEventListener('click', (event) => {
            event.preventDefault();
            if (confirm('Tem certeza que deseja sair?')) {
                localStorage.removeItem('authToken');
                localStorage.removeItem('userId');
                window.location.href = 'index.html';
            }
        });
    }

    fetchDashboardData();
    // setInterval(fetchDashboardData, 30000);
});