
iconsdir=$(datadir)/icons/hicolor
appicons16dir=$(iconsdir)/16x16/apps
appicons24dir=$(iconsdir)/24x24/apps
appicons32dir=$(iconsdir)/32x32/apps
appicons48dir=$(iconsdir)/48x48/apps
appicons128dir=$(iconsdir)/128x128/apps
appiconsscalabledir=$(iconsdir)/scalable/apps

uiiconsdir=$(datadir)/poedit/icons
uiiconssymbolicdir=$(uiiconsdir)/hicolor/scalable/actions
uiiconssymbolicyarudir=$(uiiconsdir)/Yaru/scalable/actions

dist_appicons16_DATA = linux/appicon/16x16/apps/net.poedit.Poedit.png
dist_appicons24_DATA = linux/appicon/24x24/apps/net.poedit.Poedit.png
dist_appicons32_DATA = linux/appicon/32x32/apps/net.poedit.Poedit.png
dist_appicons48_DATA = linux/appicon/48x48/apps/net.poedit.Poedit.png
dist_appicons128_DATA = linux/appicon/128x128/apps/net.poedit.Poedit.png
dist_appiconsscalable_DATA = linux/appicon/scalable/apps/net.poedit.Poedit.svg

dist_uiiconssymbolic_DATA = \
		linux/poedit-pretranslate-symbolic.svg \
		linux/poedit-sync-symbolic.svg \
		linux/poedit-update-symbolic.svg \
		linux/poedit-upload-symbolic.svg \
		linux/poedit-validate-symbolic.svg \
		linux/poedit-sidebar-symbolic.svg

dist_uiiconssymbolicyaru_DATA = \
		linux/yaru/poedit-pretranslate-symbolic.svg \
		linux/yaru/poedit-sync-symbolic.svg \
		linux/yaru/poedit-update-symbolic.svg \
		linux/yaru/poedit-upload-symbolic.svg \
		linux/yaru/poedit-validate-symbolic.svg \
		linux/yaru/poedit-sidebar-symbolic.svg

dist_uiicons_DATA = \
		AccountCrowdin.png \
		AccountLocalazy.png \
		CrowdinLogo.png \
		CrowdinLogoDark.png \
		LocalazyLogo.png \
		DownvoteTemplate.png \
		EmptyMultiSelectionTemplate.png \
		ExtractorsGNUgettext.png \
		ItemCommentTemplate.png \
		SuggestionErrorTemplate.png \
		SuggestionPerfectMatch.png \
		SuggestionTMTemplate.png \
		poedit-status-cat-mid.png \
		poedit-status-cat-no.png \
		poedit-status-cat-ok.png \
		StatusError.png \
		StatusErrorBlack.png \
		StatusWarning.png \
		StatusWarningBlack.png \
		window-close.png

update-icon-cache:
	test -z $(DESTDIR) && which gtk-update-icon-cache >/dev/null && gtk-update-icon-cache -f -t $(iconsdir) || true

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
