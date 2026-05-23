import sys
import os

# Dynamically find the bundled site-packages directory inside your app bundle
current_dir = os.path.dirname(os.path.abspath(__file__))
runtime_lib_path = os.path.abspath(os.path.join(current_dir, "..", "python_runtime", "site-packages"))
if runtime_lib_path not in sys.path:
    sys.path.append(runtime_lib_path)

import yt_dlp

def get_audio_url(url):
    ydl_opts = {
        'format': 'bestaudio',
        'skip_download': True,
        'quiet': True,
        'no_warnings': True,
    }
    try:
        with yt_dlp.YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(url, download=False)
            if 'url' in info:
                return info['url']
            elif 'entries' in info:
                # Fallback for playlists
                return info['entries'][0]['url']
            return "ERROR: No direct stream URL extracted"
    except Exception as e:
        return f"ERROR: {str(e)}"
