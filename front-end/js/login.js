document.addEventListener('DOMContentLoaded', () => {
    const loginForm = document.getElementById('loginForm');
    const registerButton = document.getElementById('registerBtn');
    const forgotPasswordLink = document.getElementById('forgotPasswordLink');
    const rememberMeCheckbox = document.getElementById('rememberMe');
    const emailInput = document.getElementById('email');
    const passwordInput = document.getElementById('password');
    const messageContainer = document.getElementById('messageContainer');

    function showMessage(type, text) {
        messageContainer.style.display = 'block';
        messageContainer.className = `message ${type}`;
        messageContainer.textContent = text;
        setTimeout(() => {
            messageContainer.style.display = 'none';
        }, 5000);
    }

    if (localStorage.getItem('rememberedEmail')) {
        emailInput.value = localStorage.getItem('rememberedEmail');
        rememberMeCheckbox.checked = true;
    }

    if (loginForm) {
        loginForm.addEventListener('submit', async (event) => {
            event.preventDefault();

            const email = emailInput.value;
            const password = passwordInput.value;
            const rememberMe = rememberMeCheckbox.checked;

            messageContainer.style.display = 'none';

            if (!email || !password) {
                showMessage('error', 'Por favor, preencha todos os campos.');
                return;
            }
            if (!email.includes('@')) {
                showMessage('error', 'Por favor, insira um email válido.');
                return;
            }

            if (rememberMe) {
                localStorage.setItem('rememberedEmail', email);
            } else {
                localStorage.removeItem('rememberedEmail');
            }

            // SIMULAÇÃO DO LOGIN:
            // Para fins de demonstração do Front-end sem o Back-end, vamos simular o sucesso.
            // Em um ambiente real, esta parte faria uma requisição à sua API.

            const TEST_EMAIL = 'teste@exemplo.com';
            const TEST_PASSWORD = 'password123';

            if (email === TEST_EMAIL && password === TEST_PASSWORD) {
                showMessage('success', 'Login realizado com sucesso! Redirecionando...');
                localStorage.setItem('authToken', 'mock_auth_token_123'); // Token de teste
                localStorage.setItem('userId', 'mock_user_id_456');

                setTimeout(() => {
                    window.location.href = 'dashboard.html';
                }, 1500);
            } else {
                // Se as credenciais de teste não baterem
                showMessage('error', 'Credenciais inválidas. Use "teste@exemplo.com" e "password123".');
            }

            // A parte original com `fetch` (comentada para simulação):
            /*
            const LOGIN_API_URL = 'SUA_URL_DA_API_DE_LOGIN'; // Substitua pela URL real

            try {
                const response = await fetch(LOGIN_API_URL, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ email, password }),
                });

                const data = await response.json();

                if (response.ok) {
                    showMessage('success', data.message || 'Login realizado com sucesso! Redirecionando...');
                    if (data.token) {
                        localStorage.setItem('authToken', data.token);
                    }
                    if (data.userId) {
                        localStorage.setItem('userId', data.userId);
                    }
                    
                    setTimeout(() => {
                        window.location.href = 'dashboard.html';
                    }, 1500);

                } else {
                    showMessage('error', data.message || 'Credenciais inválidas. Verifique seu email e senha.');
                }
            } catch (error) {
                showMessage('error', 'Erro de conexão. Tente novamente mais tarde ou verifique sua internet.');
            }
            */
        });
    }

    if (registerButton) {
        registerButton.addEventListener('click', () => {
            window.location.href = 'register-device.html';
        });
    }

    if (forgotPasswordLink) {
        forgotPasswordLink.addEventListener('click', (event) => {
            event.preventDefault();
            alert('Você será redirecionado para a página de recuperação de senha.');
            // window.location.href = 'recuperar-senha.html'; // Descomentar e ajuste quando tiver a página
        });
    }
});