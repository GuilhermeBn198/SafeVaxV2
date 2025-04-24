# whatsapp_service.py
import os
import requests

PHONE_ID   = os.getenv("WHATSAPP_PHONE_NUMBER_ID")  # fornecido pela Meta
ACCESS_TOKEN = os.getenv("WHATSAPP_ACCESS_TOKEN")   # token temporário ou permanente

API_URL = f"https://graph.facebook.com/v18.0/{PHONE_ID}/messages"

def send_whatsapp_message(body: str, to: str) -> bool:
    headers = {
        "Authorization": f"Bearer {ACCESS_TOKEN}",
        "Content-Type": "application/json"
    }

    payload = {
        "messaging_product": "whatsapp",
        "to": to,
        "type": "text",
        "text": {"body": body}
    }

    try:
        resp = requests.post(API_URL, headers=headers, json=payload)
        if resp.ok:
            print("[WhatsApp] Mensagem enviada com sucesso.")
            return True
        else:
            print(f"[WhatsApp] Erro {resp.status_code}: {resp.text}")
            return False
    except Exception as e:
        print("[WhatsApp] Exceção ao enviar:", e)
        return False
