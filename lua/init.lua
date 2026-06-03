-- Given the link to a youtube URL, return an URL to an audio stream.
-- Pinning a single signature-free player client (android_vr) and skipping
-- the HLS/DASH manifest probes roughly halves extraction time vs yt-dlp's
-- default multi-client negotiation (~2.5s -> ~1.2s).
function on_parse_youtube (url)
	local args = "--extractor-args 'youtube:player_client=android_vr;skip=hls,dash,translated_subs' --no-playlist"
	local result = cm_execute("yt-dlp " .. args .. " -f '140/bestaudio' --get-url '" .. url .. "' 2>/dev/null")
	-- trim trailing whitespace/newline
	result = result:match("^(.-)%s*$")
	return result or ""
end

-- Called when screen needs layout
function on_layout (width, height, ppi)
	-- print("LUA LAYOUT");
end

function on_select_plugin (filename, plugins)
	if string.find(filename, '.mod', 1, true) then
		for p in plugins do
			if p == 'uade' then
				return p;
			end
		end
	end
	return nil
end
