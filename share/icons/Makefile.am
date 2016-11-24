desktopdir = $(datadir)/applications
desktop_DATA = trackballs.desktop


trackballs.desktop: trackballs.desktop.in
	cp trackballs.desktop.in trackballs.desktop
	echo "Exec=$(bindir)/trackballs" >> trackballs.desktop


iconsdir = $(datadir)/icons/hicolor
icons_DATA = trackballs-32x32.png trackballs-48x48.png trackballs-64x64.png

EXTRA_DIST = $(desktop_DATA) $(icons_DATA) trackballs.desktop.in

install-iconsDATA:
	$(mkinstalldirs) $(DESTDIR)$(iconsdir)/32x32/apps
	$(INSTALL_DATA) $(srcdir)/trackballs-32x32.png $(DESTDIR)$(iconsdir)/32x32/apps/trackballs.png
	$(mkinstalldirs) $(DESTDIR)$(iconsdir)/48x48/apps
	$(INSTALL_DATA) $(srcdir)/trackballs-48x48.png $(DESTDIR)$(iconsdir)/48x48/apps/trackballs.png
	$(mkinstalldirs) $(DESTDIR)$(iconsdir)/64x64/apps
	$(INSTALL_DATA) $(srcdir)/trackballs-64x64.png $(DESTDIR)$(iconsdir)/64x64/apps/trackballs.png


uninstall-iconsDATA:
	-rm -f $(iconsdir)/32x32/apps/trackballs.png
	-rm -f $(iconsdir)/48x48/apps/trackballs.png
	-rm -f $(iconsdir)/64x64/apps/trackballs.png
