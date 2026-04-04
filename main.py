import serial
import time

# --- CONFIGURAÇÕES ---
porta_serial = 'COM3'  # No Windows ex: 'COM3', no Linux/Mac ex: '/dev/ttyUSB0'
baud_rate = 9600
tensao_rede = 127.0    # Ajuste para 127 ou 220 conforme sua região
fator_potencia = 1.0   # 1.0 para cargas resistivas (lâmpadas incand., resistências)

# Variáveis de Acúmulo
consumo_wh = 0.0
tempo_anterior = time.time()

try:
    # Inicializa a conexão Serial
    ser = serial.Serial(porta_serial, baud_rate, timeout=1)
    print(f"Conectado em {porta_serial}. Aguardando dados...")
    print("Pressione Ctrl+C para parar e ver o resumo.")
    
    while True:
        if ser.in_waiting > 0:
            # 1. Lê a linha da Serial
            linha = ser.readline().decode('utf-8').strip()
            
            # 2. Filtra apenas a linha que contém o valor da corrente
            if "Corrente_A:" in linha:
                try:
                    # Extrai o valor numérico após o ":"
                    corrente_str = linha.split(":")[1]
                    corrente = float(corrente_str)
                    
                    # 3. Cálculo de Potência Ativa (P = V * I * FP)
                    potencia_w = tensao_rede * corrente * fator_potencia
                    
                    # 4. Cálculo do Intervalo de Tempo (Delta T)
                    tempo_atual = time.time()
                    delta_tempo_segundos = tempo_atual - tempo_anterior
                    tempo_anterior = tempo_atual
                    
                    # 5. Cálculo de Energia (E = P * t)
                    # Convertendo segundos para horas (1h = 3600s)
                    energia_segmento_wh = (potencia_w * delta_tempo_segundos) / 3600
                    consumo_wh += energia_segmento_wh
                    consumo_kwh = consumo_wh / 1000
                    
                    # Exibição no Console
                    print(f"Corrente: {corrente:.3f} A | Potência: {potencia_w:.1f} W | Total: {consumo_kwh:.6f} kWh")
                    
                except ValueError:
                    continue

except KeyboardInterrupt:
    print("\n--- Monitoramento Encerrado ---")
    print(f"Consumo Total Acumulado: {consumo_wh:.4f} Wh")
    print(f"Consumo Total Acumulado: {consumo_wh/1000:.6f} kWh")
    ser.close()

except Exception as e:
    print(f"Erro: {e}")