#include <EEPROM.h>

// Pinos e Calibração do Sensor (Definidos em testes anteriores)
const int pinSensorCorrente = A0;
float offsetCorrente = 510.0;
float fatorCalibracao = 0.0211; // Fator temporário para leitura de baixas correntes
float tensaoRede = 129.0;       // Tensão medida por você na rede local

// Constantes de tempo
const unsigned long tempoAmostragem = 100; 
const unsigned long INTERVALO_SALVAR = 3600000; // 60 minutos em milissegundos

// Variáveis dinâmicas para o cálculo de Consumo
float consumoKWh = 0.0;
unsigned long tempoUltimoCalculo = 0;
unsigned long tempoUltimoSalvamento = 0;

// Configurações da Memória EEPROM (Buffer Circular de 7 dias)
const int ENDERECO_INDICE = 0;        // Primeiros 2 bytes reservamos para salvar a posição do ponteiro
const int ENDERECO_INICIO_DADOS = 2;  // Os dados de consumo começam a partir do byte 2
const int MAX_REGISTROS = 168;        // Limite de 168 horas (7 dias) para rotacionar
int indiceEEPROM = 0;

void setup() {
  Serial.begin(9600);
  
  // 1. Inicialização e Recuperação da EEPROM
  EEPROM.get(ENDERECO_INDICE, indiceEEPROM);
  
  // Validação de segurança: se a memória for virgem (lixo), reseta o índice
  if (indiceEEPROM < 0 || indiceEEPROM >= MAX_REGISTROS) {
    indiceEEPROM = 0;
    EEPROM.put(ENDERECO_INDICE, indiceEEPROM);
  } else {
    // Se o índice for válido, recupera o último consumo salvo
    int enderecoUltimoDado = ENDERECO_INICIO_DADOS + (indiceEEPROM * sizeof(float));
    EEPROM.get(enderecoUltimoDado, consumoKWh);
    
    // Tratamento de segurança caso o dado lido seja NaN (Not a Number) ou negativo
    if (isnan(consumoKWh) || consumoKWh < 0.0) {
      consumoKWh = 0.0;
    }
  }
  
  Serial.println("--- Sistema Iniciado ---");
  Serial.println("Recuperando histórico de sobrevivência...");
  Serial.print("Ponteiro da Memória: "); Serial.println(indiceEEPROM);
  Serial.print("Consumo Recuperado: "); Serial.print(consumoKWh, 4); Serial.println(" kWh");
  Serial.println("------------------------");
  
  tempoUltimoCalculo = millis();
  tempoUltimoSalvamento = millis();
}

void loop() {
  unsigned long tempoInicio = millis();
  long qtdAmostras = 0;
  float acumuladorCorrente = 0.0;
  float valorCentralizado = 0.0;

  // 1. Loop de Amostragem em Alta Velocidade
  while ((millis() - tempoInicio) < tempoAmostragem) {
    int leituraADC = analogRead(pinSensorCorrente);
    valorCentralizado = leituraADC - offsetCorrente;
    acumuladorCorrente += (valorCentralizado * valorCentralizado);
    qtdAmostras++;
  }

  // 2. Processamento Digital do Sinal (Cálculo RMS)
  float mediaQuadrados = acumuladorCorrente / (float)qtdAmostras;
  float rmsBruto = sqrt(mediaQuadrados);
  float correnteRMS = rmsBruto * fatorCalibracao;

  // Filtro de Zona Morta (Remove ruídos de fundo)
  if (correnteRMS < 0.02) {
    correnteRMS = 0.0;
  }
  
  // 3. Hodômetro: Integração do Consumo Acumulado (kWh)
  unsigned long tempoAtual = millis();
  // Converte a diferença de tempo em milissegundos para horas exatas (em frações float)
  float horasPassadas = (tempoAtual - tempoUltimoCalculo) / 3600000.0; 
  float potenciaKW = (correnteRMS * tensaoRede) / 1000.0;
  
  consumoKWh += (potenciaKW * horasPassadas);
  tempoUltimoCalculo = tempoAtual;

  // 4. Ponte Serial: Envio dos dados para o Python interceptar
  Serial.print("Corrente_A:"); Serial.println(correnteRMS, 3);
  Serial.print("Potencia_kW:"); Serial.println(potenciaKW, 4);
  Serial.print("Consumo_kWh:"); Serial.println(consumoKWh, 6);

  // 5. Persistência de Sobrevivência na EEPROM (Apenas a cada 60 minutos)
  if (tempoAtual - tempoUltimoSalvamento >= INTERVALO_SALVAR) {
    // Rotaciona o buffer: se chegar a 167, o próximo (% 168) volta a ser 0
    indiceEEPROM = (indiceEEPROM + 1) % MAX_REGISTROS;
    
    // Define o endereço físico multiplicando o índice pelo peso de um float (4 bytes)
    int enderecoGravacao = ENDERECO_INICIO_DADOS + (indiceEEPROM * sizeof(float));
    
    // Grava o novo consumo e salva a nova posição do ponteiro
    EEPROM.put(enderecoGravacao, consumoKWh);
    EEPROM.put(ENDERECO_INDICE, indiceEEPROM); 
    
    Serial.println("--- CHECKPOINT: DADOS SALVOS NA EEPROM ---");
    tempoUltimoSalvamento = tempoAtual;
  }

  delay(1000); // Aguarda um segundo entre leituras
}