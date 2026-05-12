// Definição do pino analógico onde o Fio B do SCT-013 está conectado
const int pinSensorCorrente = A0;

// Variáveis de Calibração
// O offset ideal de 2.5V no Arduino (0 a 5V em conversor de 10 bits) é 512.
// Este valor pode variar levemente dependendo dos resistores reais.
// Ajustado para 510 com base em observação de valores lidos entre 507 e 513
float offsetCorrente = 510.0; 

// Fator de calibração que você ajustará na Quinzena 4 usando o multímetro
float fatorCalibracao = 0.2077; 

// Período de amostragem para capturar ciclos completos de 60Hz.
// Ex: 100 ms equivale a 6 ciclos completos da rede elétrica (16.67ms * 6)
const unsigned long tempoAmostragem = 100; 

void setup() {
  // Inicializa a comunicação serial a 9600 bps para o script Python ler
  Serial.begin(9600);
}

void loop() {
  unsigned long tempoInicio = millis();
  long qtdAmostras = 0;
  float acumuladorCorrente = 0.0;
  float valorCentralizado = 0.0;

  // 1. Loop de Amostragem (Captura dos dados brutos no tempo)
  while ((millis() - tempoInicio) < tempoAmostragem) {
    // Lê o valor do conversor Analógico-Digital (ADC) - valor entre 0 e 1023
    int leituraADC = analogRead(pinSensorCorrente);
    
    // 2. Filtro Digital: Remoção do nível DC (Offset)
    valorCentralizado = leituraADC - offsetCorrente;
    
    // 3. DSP: Eleva o valor ao quadrado e soma ao acumulador
    acumuladorCorrente += (valorCentralizado * valorCentralizado);
    qtdAmostras++;
  }

  // 4. Calcula a média da soma dos quadrados
  float mediaQuadrados = acumuladorCorrente / (float)qtdAmostras;

  // 5. Extrai a Raiz Quadrada (Root) da Média (Mean) dos Quadrados (Square)
  float rmsBruto = sqrt(mediaQuadrados);

  // 6. Conversão para Amperes utilizando o fator de calibração
  float correnteRMS = rmsBruto * fatorCalibracao;

  // Filtro simples para remover ruído em leituras muito baixas
  if (correnteRMS < 0.04) {
    correnteRMS = 0.0;
  }

  // 7. Envio do dado processado pela Porta Serial (formato texto/JSON)
  Serial.print("Corrente_A:");
  Serial.println(correnteRMS);
  

  // Aguarda 1 segundo antes da próxima leitura para não sobrecarregar o Python
  delay(1000); 
}