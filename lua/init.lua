-- Given the link to a youtube URL, return an URL to an audio stream
function on_parse_youtube (url)
	print("--- YouTube Native Extraction Started ---")
	print("URL: " .. url)
	
	-- Invoke our high-speed, in-memory embedded Python interpreter
	local result = cm_python_extract(url)

	url = ''
	if result and result ~= "" and not result:find("ERROR") then
		print("Extraction SUCCESS via Embedded Python Engine")
		
		-- Parse the result lines if yt-dlp happens to dump multiple formats
		for l in result:gmatch("[^\r\n]+") do
			if string.find(l, 'mime=audio', 1, true) then
				url = l
				break
			end
			if string.find(l, 'audio', 1, true) then
				url = l
			end
		end
		
		-- Fallback: If it's a single raw URL without tags, use it directly
		if url == '' then
			url = result
		end
	else
		print("Extraction FAILED: " .. (result or "Empty/Nil response from engine"))
	end
	
	if url ~= "" then
		print("Final Stream URL found: " .. url:sub(1, 50) .. "...")
	else
		print("Final Result: FAILED to resolve target audio stream")
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
