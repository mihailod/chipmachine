
-- Given the link to a youtube URL, return an URL to an audio stream
function on_parse_youtube (url)
	print("--- YouTube Extraction Started ---")
	print("URL: " .. url)
	
	local path = cm_execute("echo $PATH")
	print("Environment PATH: " .. (path or "NIL"))

	local ffmpeg_check = cm_execute("ffmpeg -version")
	if ffmpeg_check and ffmpeg_check ~= "" then
		print("FFmpeg check: OK (" .. ffmpeg_check:sub(1, 30) .. "...)")
	else
		print("FFmpeg check: FAILED")
	end

	local extractors = {
		'/opt/homebrew/bin/yt-dlp',
		'yt-dlp',
		'/usr/local/bin/yt-dlp',
		'youtube-dl'
	}

	local result = nil
	for _, ext in ipairs(extractors) do
		local cmd = string.format('%s --skip-download -g "%s"', ext, url)
		print("Attempting extraction with: " .. ext)
		result = cm_execute(cmd)
		if result and result ~= "" then
			if not result:find("HTTP Error") and not result:find("ERROR") then
				print("Extraction SUCCESS with: " .. ext)
				break
			else
				print("Extraction FAILED (Error in output) with: " .. ext)
				result = nil
			end
		else
			print("Extraction FAILED (Empty/Nil output) with: " .. ext)
		end
	end

	url = ''
	if result then
		for l in result:gmatch("[^\r\n]+") do
			if string.find(l, 'mime=audio',1 , true) then
				url = l
				break
			end
			if string.find(l, 'audio',1 , true) then
				url = l
			end
		end
	end
	
	if url ~= "" then
		print("Final Stream URL found: " .. url:sub(1, 50) .. "...")
	else
		print("Final Result: FAILED to find audio stream")
	end
	print("--- YouTube Extraction Finished ---")
	return url
	
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

