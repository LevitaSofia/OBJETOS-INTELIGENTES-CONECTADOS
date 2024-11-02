<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Monitor de Corrente ESP32</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin: 0;
            padding: 0;
            background-color: #f0f0f0;
        }
        header {
            background-color: #007BFF;
            color: white;
            padding: 10px 0;
        }
        .container {
            margin: 20px auto;
            padding: 20px;
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            max-width: 600px;
        }
        h1, h2 {
            color: #333;
        }
        .data {
            font-size: 1.5em;
            margin: 10px 0;
        }
        footer {
            margin-top: 20px;
            color: #555;
        }
    </style>
</head>
<body>
    <header>
        <h1>Monitor de Corrente</h1>
    </header>
    <div class="container">
        <h2>Dados em Tempo Real</h2>
        <p class="data"><strong>Corrente RMS:</strong> <span id="corrente">0.000</span> A</p>
        <p class="data"><strong>Potência Aparente:</strong> <span id="potencia">0</span> W</p>
        <p class="data"><strong>Endereço IP do ESP32:</strong> <span id="ip">0.0.0.0</span></p>
        <p class="data"><strong>Estado do Relé:</strong> <span id="rele">Desligado</span></p>
    </div>
    <footer>
        <p>&copy; 2024 Monitor de Energia Residencial</p>
    </footer>

    <script>
        // Simulação para a atualização dos dados (substitua isso com chamadas AJAX ou WebSocket em produção)
        function atualizarDados() {
            // Aqui você poderia usar uma requisição AJAX para buscar os dados em tempo real
            document.getElementById("corrente").textContent = "2.135"; // Exemplo de valor
            document.getElementById("potencia").textContent = "271";    // Exemplo de valor em watts
            document.getElementById("ip").textContent = "192.168.18.99"; // Exemplo de IP
            document.getElementById("rele").textContent = "Ligado";      // Exemplo de estado do relé
        }

        // Chama a função de atualização a cada 5 segundos (simula atualização em tempo real)
        setInterval(atualizarDados, 5000);
    </script>
</body>
</html>
