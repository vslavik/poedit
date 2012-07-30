

ifneq (,$(shell which convert 2>/dev/null))
CONVERT = convert
else
$(error couldn't find 'convert' -- ImageMagick is required)
endif

ifneq (,$(shell which png2icns 2>/dev/null))
  PNG2ICNS = png2icns
else
  $(error coulnd't find 'png2icns' --  libicns (http://icns.sf.net) is required)
endif

GENERATED_ICONS = \
	win32/installer_wizard_image.bmp \
	win32/appicon.ico \
	win32/xp/poedit-translation-generic.ico \
	win32/vista/poedit-translation-generic.ico \
	osx/appicon.icns \
	osx/poedit-translation-generic.icns

APPICONS_WIN32 = \
	appicon/16x16/apps/poedit.png \
	appicon/32x32/apps/poedit.png \
	appicon/48x48/apps/poedit.png \
	appicon/256x256/apps/poedit.png

APPICONS_OSX = \
	appicon/16x16/apps/poedit.png \
	appicon/32x32/apps/poedit.png \
	appicon/48x48/apps/poedit.png \
	appicon/128x128/apps/poedit.png \
	appicon/256x256/apps/poedit.png \
	appicon/512x512/apps/poedit.png


all: $(GENERATED_ICONS)

clean:
	rm -f $(GENERATED_ICONS)

win32/installer_wizard_image.bmp: appicon/48x48/apps/poedit.png
	@mkdir -p win32
	$(CONVERT) $< -background white -extent 55x55-4-4 -type palette BMP3:$@

win32/appicon.ico: $(APPICONS_WIN32)
	@mkdir -p win32
	# FIXME: should contain 8bit and 4bit versions too
	$(CONVERT) $(APPICONS_WIN32) -compress None $@

win32/xp/poedit-translation-generic.ico: mime-win32/xp/generic/*.png
	@mkdir -p win32/xp
	# FIXME: should contain 8bit and 4bit versions too
	$(CONVERT) $^ -compress None $@

win32/vista/poedit-translation-generic.ico: mime-win32/vista/generic/*.png
	@mkdir -p win32/vista
	# FIXME: should contain 8bit and 4bit versions too
	$(CONVERT) $^ -compress None $@


osx/appicon.icns: $(APPICONS_OSX)
	@mkdir -p osx
	$(PNG2ICNS) $@ $^

osx/poedit-translation-generic.icns: mime-osx/generic/*.png
	@mkdir -p osx
	$(PNG2ICNS) $@ $^
