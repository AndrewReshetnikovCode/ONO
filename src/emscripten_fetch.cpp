#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include "LawnApp.h"

extern LawnApp* gLawnApp;

static void apply_canvas_css()
{
#if defined(CANVAS_WIDTH) && defined(CANVAS_HEIGHT)
	EM_ASM({
		var c = document.getElementById('canvas');
		if (c) {
			c.style.setProperty('width', $0 + 'px', 'important');
			c.style.setProperty('height', $1 + 'px', 'important');
		}
	}, CANVAS_WIDTH, CANVAS_HEIGHT);
#endif
}

static void run_game()
{
	gLawnApp->Init();
	apply_canvas_css();
	gLawnApp->Start();
	gLawnApp->Shutdown();
	if (gLawnApp) {
		delete gLawnApp;
		gLawnApp = nullptr;
	}
}

extern "C" {

EM_JS(void, pvz_fetch_resources_and_run, (), {
	var base = '.';
	var required = [
		{ url: base + '/main.pak', path: '/main.pak' },
		{ url: base + '/properties/resources.xml', path: '/properties/resources.xml' }
	];
	var optional = [
		{ url: base + '/properties/partner.xml', path: '/properties/partner.xml' },
		{ url: base + '/properties/partner_logo.jpg', path: '/properties/partner_logo.jpg' },
		{ url: base + '/Properties/LawnStrings.txt', path: '/Properties/LawnStrings.txt' }
	];

	function fetchFile(f) {
		return fetch(f.url).then(function(r) {
			if (r.ok) {
				return r.arrayBuffer().then(function(buf) {
					var dir = f.path.substring(0, f.path.lastIndexOf('/'));
					if (dir && dir.length > 1) {
						var rel = dir.substring(1);
						try { FS.createPath('/', rel, true, true); } catch (e) {}
					}
					FS.writeFile(f.path, new Uint8Array(buf));
					return true;
				});
			}
			return false;
		});
	}

	function load() {
		var p = required.reduce(function(acc, f) {
			return acc.then(function() { return fetchFile(f).then(function(ok) {
				if (!ok) { console.error('Required file not found: ' + f.url); throw new Error('missing'); }
			}); });
		}, Promise.resolve());
		optional.forEach(function(f) {
			p = p.then(function() { return fetchFile(f).catch(function() { return false; }); });
		});
		p.then(function() { Module._pvz_run_game(); }).catch(function(e) { console.error('Failed to load resources:', e); });
	}
	load();
});

void EMSCRIPTEN_KEEPALIVE pvz_run_game()
{
	run_game();

	// Signal Yandex Games that loading is complete
	EM_ASM({
		if (typeof window !== 'undefined' && window.YG && window.YG.signalReady)
			window.YG.signalReady();
	});
}

void EMSCRIPTEN_KEEPALIVE pvz_yg_on_ad_open()
{
	if (gLawnApp) gLawnApp->Mute(true);
}

void EMSCRIPTEN_KEEPALIVE pvz_yg_on_ad_close()
{
	if (gLawnApp) gLawnApp->Mute(false);
}

void EMSCRIPTEN_KEEPALIVE pvz_yg_on_visibility_hidden()
{
	if (gLawnApp) gLawnApp->Mute(true);
}

void EMSCRIPTEN_KEEPALIVE pvz_yg_on_visibility_visible()
{
	if (gLawnApp) gLawnApp->Mute(false);
}

} // extern "C"

#endif // __EMSCRIPTEN__
