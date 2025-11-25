"""
Simple Spotify Token Getter

This script authenticates with Spotify and saves the access token to token.txt
for use with the Muteify C program.

Usage:
    python3 get_token.py
"""

import requests
from flask import Flask, request, redirect

app = Flask(__name__)

# Your Spotify app credentials
CLIENT_ID = "33b03ea6126f41b08f3c9e69fe105855"
CLIENT_SECRET = "35d99272964642609d0d7136cddfde46"
REDIRECT_URI = "http://127.0.0.1:8888/callback"

@app.route("/")
def login():
    """Redirects user to Spotify login page"""
    auth_url = (
        f"https://accounts.spotify.com/authorize"
        f"?client_id={CLIENT_ID}"
        f"&response_type=code"
        f"&redirect_uri={REDIRECT_URI}"
        f"&scope=user-read-playback-state"
    )
    return redirect(auth_url)


@app.route("/callback")
def callback():
    """Handles Spotify callback and saves access token"""
    code = request.args.get("code")
    
    if not code:
        return "Error: No authorization code received"
    
    # Exchange code for access token
    token_url = "https://accounts.spotify.com/api/token"
    request_body = {
        "code": code,
        "grant_type": "authorization_code",
        "redirect_uri": REDIRECT_URI,
        "client_id": CLIENT_ID,
        "client_secret": CLIENT_SECRET,
    }
    
    response = requests.post(token_url, data=request_body, timeout=100)
    token_info = response.json()
    
    access_token = token_info.get("access_token")
    
    if access_token:
        # Save token to file
        with open("token.txt", "w") as f:
            f.write(access_token)
        
        return "<h1>✓ Success!</h1><p>Token saved to token.txt</p><p>You can close this window and stop the server (Ctrl+C)</p>"
    else:
        return f"<h1>Error</h1><p>Failed to get access token</p><pre>{token_info}</pre>"


if __name__ == "__main__":
    print("Spotify Token Getter")
    print("1. Starting server on http://127.0.0.1:8888")
    print("3. Log in to Spotify")
    print("4. Token will be saved to token.txt")
    
    app.run(port=8888)