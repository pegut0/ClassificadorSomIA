import requests
import os

# --- Configurações ---
# URL do seu servidor local. 
# Se o servidor e o cliente estão na mesma máquina, 127.0.0.1 (localhost) funciona perfeitamente.
SERVER_URL = "http://127.0.0.1:5000/predict"

# Nome do arquivo de áudio que você quer testar.
# O arquivo deve estar na mesma pasta que este script.
# Mude este nome para testar outros sons!
FILENAME_TO_TEST = "baby.wav"

# --- Função Principal de Teste ---
def test_audio_classifier(file_path):
    """
    Envia um arquivo de áudio para o servidor Flask e imprime a resposta.
    """
    print(f"▶️  Tentando enviar o arquivo: {file_path}")

    # Verifica se o arquivo de áudio existe
    if not os.path.exists(file_path):
        print(f"❌ ERRO: Arquivo não encontrado em '{file_path}'")
        print("Por favor, verifique se o nome do arquivo está correto e se ele está na mesma pasta que o script.")
        return

    try:
        # Abre o arquivo de áudio em modo de leitura binária ('rb')
        with open(file_path, 'rb') as audio_file:
            # Prepara o arquivo para ser enviado na requisição.
            # A chave 'audio' deve ser a mesma que o servidor espera (request.files['audio'])
            files_to_send = {'audio': (os.path.basename(file_path), audio_file, 'audio/wav')}
            
            # Envia a requisição POST para o servidor
            response = requests.post(SERVER_URL, files=files_to_send)

            # Verifica o código de status da resposta
            response.raise_for_status()  # Isso vai gerar um erro para códigos como 4xx ou 5xx

            # Se a requisição foi bem-sucedida, extrai e imprime apenas a predição principal
            print("✅ Resposta recebida do servidor:")
            data = response.json()
            prediction = data.get('prediction', 'N/A')
            confidence = data.get('confidence', 'N/A')
            
            print(f"   ➡️  Predição Principal: {prediction}")

    except requests.exceptions.RequestException as e:
        print(f"❌ ERRO DE CONEXÃO: Não foi possível conectar ao servidor em {SERVER_URL}")
        print(f"   Detalhes: {e}")
        print("   Verifique se o seu servidor Flask está rodando.")
    except Exception as e:
        print(f"❌ Ocorreu um erro inesperado: {e}")


if __name__ == '__main__':
    # Roda a função de teste com o nome de arquivo definido acima
    test_audio_classifier(FILENAME_TO_TEST)
