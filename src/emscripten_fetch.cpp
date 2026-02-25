#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include "LawnApp.h"

extern LawnApp* gLawnApp;

static void run_game()
{
	gLawnApp->Init();
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
}

} // extern "C"

#endif // __EMSCRIPTEN__
