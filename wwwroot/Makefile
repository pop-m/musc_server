.PHONY: download_mp3 clean
src = music_download.c
bin = download_mp3
cc = gcc

$(bin):$(src)
	$(cc) $< -o $@

clean:
	rm $(bin)
