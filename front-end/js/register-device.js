document.addEventListener('DOMContentLoaded', () => {
    const registerDeviceForm = document.getElementById('registerDeviceForm');
    const minHumidityInput = document.getElementById('minHumidity');
    const minHumidityValueSpan = document.getElementById('minHumidityValue');
    const cancelBtn = document.getElementById('cancelBtn');
    const messageContainer = document.getElementById('messageContainer');
    const logoutLink = document.getElementById('logoutLink');
    const profileLink = document.getElementById('profileLink');

    function showMessage(type, text) {
        messageContainer.style.display = 'block';
        messageContainer.className = `message ${type}`;
        messageContainer.textContent = text;
        setTimeout(() => {
            messageContainer.style.display = 'none';
        }, 5000);
    }

    if (minHumidityInput && minHumidityValueSpan) {
        minHumidityValueSpan.textContent = `${minHumidityInput.value}%`;
        minHumidityInput.addEventListener('input', () => {
            minHumidityValueSpan.textContent = `${minHumidityInput.value}%`;
        });
    }

    if (registerDeviceForm) {
        registerDeviceForm.addEventListener('submit', async (event) => {
            event.preventDefault();

            messageContainer.style.display = 'none';

            const deviceData = {
                deviceName: document.getElementById('deviceName').value,
                macAddress: document.getElementById('macAddress').value,
                gardenLocation: document.getElementById('gardenLocation').value,
                associatedCulture: document.getElementById('associatedCulture').value,
                automationParams: {
                    minHumidity: parseInt(minHumidityInput.value),
                    wateringDuration: parseInt(document.getElementById('wateringDuration').value),
                    minIntervalHours: parseInt(document.getElementById('minInterval').value),
                },
                deviceStatus: document.querySelector('input[name="deviceStatus"]:checked').value,
            };

            if (
                !deviceData.deviceName ||
                !deviceData.macAddress ||
                !deviceData.gardenLocation ||
                !deviceData.associatedCulture ||
                isNaN(deviceData.automationParams.minHumidity) ||
                isNaN(deviceData.automationParams.wateringDuration) ||
                isNaN(deviceData.automationParams.minIntervalHours)
            ) {
                showMessage('error', 'Por favor, preencha todos os campos obrigatórios.');
                return;
            }

            // SIMULAÇÃO DO CADASTRO DE DISPOSITIVO:
            // Em um ambiente real, esta parte faria uma requisição à sua API.

            const authToken = localStorage.getItem('authToken');
            if (!authToken) {
                showMessage('error', 'Você não está autenticado. Faça login novamente.');
                setTimeout(() => window.location.href = 'index.html', 1500);
                return;
            }

            // Simula um atraso de rede
            await new Promise(resolve => setTimeout(resolve, 1000));

            showMessage('success', 'Dispositivo cadastrado com sucesso (simulado)! Redirecionando...');
            registerDeviceForm.reset();
            minHumidityValueSpan.textContent = '50%';
            
            setTimeout(() => {
                window.location.href = 'dashboard.html';
            }, 2000);

            // A parte original com `fetch` (comentada para simulação):
            /*
            const REGISTER_DEVICE_API_URL = 'SUA_URL_DA_API_DE_CADASTRO_DE_DISPOSITIVO';

            try {
                const response = await fetch(REGISTER_DEVICE_API_URL, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        'Authorization': `Bearer ${authToken}`,
                    },
                    body: JSON.stringify(deviceData),
                });

                const data = await response.json();

                if (response.ok) {
                    showMessage('success', data.message || 'Dispositivo cadastrado com sucesso!');
                    registerDeviceForm.reset();
                    minHumidityValueSpan.textContent = '50%';
                    setTimeout(() => {
                        window.location.href = 'dashboard.html';
                    }, 2000);
                } else {
                    showMessage('error', data.message || 'Erro ao cadastrar dispositivo. Tente novamente.');
                }
            } catch (error) {
                showMessage('error', 'Erro de conexão. Não foi possível cadastrar o dispositivo.');
            }
            */
        });
    }

    if (cancelBtn) {
        cancelBtn.addEventListener('click', () => {
            if (confirm('Tem certeza que deseja cancelar? As informações não salvas serão perdidas.')) {
                window.location.href = 'dashboard.html';
            }
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

    if (profileLink) {
        profileLink.addEventListener('click', (event) => {
            event.preventDefault();
            alert('Funcionalidade de perfil em desenvolvimento!');
        });
    }
});