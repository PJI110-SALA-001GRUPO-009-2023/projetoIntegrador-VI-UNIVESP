document.addEventListener('DOMContentLoaded', () => {
    const gardenStatusElement = document.getElementById('gardenStatus');
    const lastWateringTimeElement = document.getElementById('lastWateringTime');
    const soilMoistureElement = document.getElementById('soilMoisture');
    const airTempCElement = document.getElementById('airTempC');
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

        const DASHBOARD_API_URL = 'https://irrigadror-inteligente.azurewebsites.net/api/getLastData/';

        try {
            const response = await fetch(DASHBOARD_API_URL, { method: 'GET' });
            if (!response.ok) {
                throw new Error(`Network response was not ok (${response.status})`);
            }

            const respJson = await response.json();

            let decodedBody = null;
            if (respJson && respJson.Body) {
                decodedBody = atob(respJson.Body);
            }

            let dataFromApi = null;
            try {
                dataFromApi = decodedBody ? JSON.parse(decodedBody) : null;
            } catch (err) {
                dataFromApi = null;
            }

            const fallbackData = {
                generalStatus: 'Sua horta está saudável e produtiva!',
                soilMoisture: 72,
                airTempC: 26,
                airHumidity: 85,
                lastWatering: new Date(new Date().setHours(new Date().getHours() - 3, 0, 0)).toISOString(),
                lastReading: new Date().toISOString(),
            };

            const finalData = Object.assign({}, fallbackData, dataFromApi || {});

            const sysProps = respJson && (respJson.SystemProperties || respJson.systemProperties);
            if (sysProps && sysProps["iothub-enqueuedtime"]) {
                try {
                    const enqueuedUtc = new Date(sysProps["iothub-enqueuedtime"]).toISOString();
                    const gmtMinus3 = new Date(new Date(enqueuedUtc).getTime());
                    finalData.lastReading = gmtMinus3.toISOString();
                } catch (err) {

                }
            }

            updateDashboardUI(finalData);
        } catch (error) {
            alert('Erro ao carregar dados do dashboard: ' + (error.message || 'Erro desconhecido.'));
            
            const fallbackData = {
                generalStatus: 'Sua horta está saudável e produtiva!',
                soilMoisture: 72,
                airTempC: 26,
                airHumidity: 85,
                lastWatering: new Date(new Date().setHours(new Date().getHours() - 3, 0, 0)).toISOString(),
                lastReading: new Date().toISOString(),
            };
            updateDashboardUI(fallbackData);
        }
    }

    function updateDashboardUI(data) {
        // remove loading classes before setting real content
        [lastWateringTimeElement, soilMoistureElement, airTempCElement, airHumidityElement, lastReadingTimeElement].forEach(el => {
            if (el) el.classList.remove('loading');
        });

        gardenStatusElement.textContent = data.generalStatus || 'Carregando...';
        soilMoistureElement.textContent = `${data.soilMoisture || 0}%`;
        airTempCElement.textContent = `${data.airTempC || 0}°C`;
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