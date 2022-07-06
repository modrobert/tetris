CC = g++
CFLAGS = -DAUDIO -std=c++11 -O2 -Wpedantic
INCLUDES = -lSDL2 -lSDL2_ttf

INSTALL_DIR = /usr/local/games/tetris
DESKTOP_DIR = ${HOME}/.local/share/applications

default: tetris

debug: CFLAGS = -DAUDIO -std=c++11 -g -Wpedantic
debug: tetris

silent: CFLAGS = -std=c++11 -O2 -Wpedantic
silent: silent_tetris

tetris.o: tetris.cc
	$(CC) $(CFLAGS) -c tetris.cc -o tetris.o $(INCLUDES)

audio.o: audio.cc audio.h
	$(CC) $(CFLAGS) -c audio.cc -o audio.o $(INCLUDES)

tetris: tetris.o audio.o
	$(CC) $(CFLAGS) tetris.o audio.o -o tetris $(INCLUDES)

silent_tetris: tetris.o
	$(CC) $(CFLAGS) tetris.o -o tetris $(INCLUDES)

install:
	mkdir -p $(INSTALL_DIR)
	echo -n "0" >$(INSTALL_DIR)/.hiscore.txt
	chmod 666 $(INSTALL_DIR)/.hiscore.txt
	mkdir -p $(INSTALL_DIR)/icon
	cp tetris $(INSTALL_DIR)/tetris
	cp icon/tetris.png $(INSTALL_DIR)/icon
	cp -r sounds $(INSTALL_DIR)
	cp -r fonts $(INSTALL_DIR)

uninstall:
	rm -f $(INSTALL_DIR)/tetris
	rm -f $(INSTALL_DIR)/.hiscore.txt 
	rm -f $(INSTALL_DIR)/icon/tetris.png
	rm -f $(INSTALL_DIR)/sounds/*
	rm -f $(INSTALL_DIR)/fonts/*
	rmdir $(INSTALL_DIR)/icon
	rmdir $(INSTALL_DIR)/sounds
	rmdir $(INSTALL_DIR)/fonts
	rmdir $(INSTALL_DIR)

install_desktop_icon:
	cp icon/tetris.desktop /tmp
	sed -i 's,xxx,$(INSTALL_DIR),g' /tmp/tetris.desktop
	rm -f $(DESKTOP_DIR)/tetris.desktop
	update-desktop-database $(DESKTOP_DIR)
	desktop-file-validate /tmp/tetris.desktop
	desktop-file-install --dir=$(DESKTOP_DIR) /tmp/tetris.desktop
	update-desktop-database $(DESKTOP_DIR)
	rm -f /tmp/tetris.desktop

uninstall_desktop_icon:
	rm -f $(DESKTOP_DIR)/tetris.desktop
	update-desktop-database $(DESKTOP_DIR)

clean:
	-rm -f audio.o
	-rm -f tetris.o
	-rm -f tetris
	
