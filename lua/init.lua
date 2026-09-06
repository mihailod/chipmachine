-- Given the link to a youtube URL, return an URL to an audio stream.
-- Pinning a single signature-free player client and skipping the HLS/DASH
-- manifest probes roughly halves extraction time vs yt-dlp's default
-- multi-client negotiation (~2.5s -> ~1.5s).
--
-- The pinned client MUST be one that does not require a GVS PO Token.
-- android_vr used to qualify and no longer does: YouTube now serves it only
-- the first ~1MB (~66s at itag 140) and returns HTTP 403 for every byte past
-- that, so any track longer than about a minute sat at "BUFFERING..." forever
-- while short clips still played. visionos is currently token-free and serves
-- the full file. If long videos start 403ing again, this pin is the first
-- thing to re-check: run yt-dlp by hand and look for a "requires a GVS PO
-- Token" warning.
--
-- Needs yt-dlp >= 2026.08: older builds do not know "visionos", silently fall
-- back to android_vr and reintroduce the 403. Caveat: "made for kids" videos
-- are not available through this client.
--
-- main.cpp PREPENDS bin/ytdlp to PATH, so the helper that actually answers is
-- the bundled one, NOT whatever is installed system-wide. Running the command
-- below by hand therefore proves nothing about the app -- it resolves a
-- different binary. Rebuild the bundled tree with ./make-ytdlp.sh.
--
-- Everything below stays INSIDE this function on purpose: package_app.sh
-- strips the comment block plus the function body for the MAS bundle with a
-- regex that needs the comments to sit directly above `function`. A global
-- between them would leave these yt-dlp comments in the shipped MAS lua and
-- trip that script's own LUA_LEAK check.
function on_parse_youtube (url)
	local client = "visionos"
	local args = "--extractor-args 'youtube:player_client=" .. client ..
	             ";skip=hls,dash,translated_subs' --no-playlist"
	local result = cm_execute("yt-dlp " .. args .. " -f '140/bestaudio' --get-url '" .. url .. "' 2>/dev/null")
	-- trim trailing whitespace/newline
	result = result:match("^(.-)%s*$") or ""

	-- Guard against a silently stale helper. googlevideo stamps the client that
	-- minted a URL into its c= parameter, so a mismatch here means yt-dlp did
	-- not understand the client we asked for and quietly fell back to another
	-- one. The URL still looks perfectly valid and short clips still play --
	-- it only 403s past the first ~1MB -- which is precisely why this failed
	-- silently for so long. Checking costs one string match and turns a
	-- mystifying "buffers forever" into a log line naming the cause.
	if result ~= "" then
		local got = result:match("[?&]c=([%w_]+)")
		if got and got:upper() ~= client:upper() then
			print("WARNING: asked yt-dlp for player_client=" .. client ..
			      " but the URL came back stamped c=" .. got .. ".")
			print("         The bundled helper (bin/ytdlp, which is FIRST on PATH)")
			print("         does not know that client and fell back. Tracks over")
			print("         ~1MB (~66s) will fail with HTTP 403. Fix: run")
			print("         ./make-ytdlp.sh to rebuild bin/ytdlp.")
		end
	end

	return result
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
