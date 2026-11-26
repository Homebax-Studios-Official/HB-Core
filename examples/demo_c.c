/* HB Core demo using only the stable C ABI. */
#include <hbcore/hb_core_c.h>
#include <stdio.h>

static void on_event(const HB_Event* e, void* user) {
    (void)user;
    if (e->type == HB_EVENT_TITLE_CHANGED) printf("title changed: %s\n", e->text);
}

int main(void) {
    HB_CoreConfig cc = {sizeof(HB_CoreConfig), 1, 0, 512, 3};
    HB_Core* core = hb_core_create(&cc);
    if (!core) return 1;
    HB_ViewConfig vc = {sizeof(HB_ViewConfig), 400, 160, 1.0f};
    HB_View* view = hb_view_create(core, &vc);
    if (!view) { hb_core_destroy(core); return 2; }
    hb_view_set_event_callback(view, on_event, NULL);
    HB_Result r = hb_view_load_html(view,
        "<html><head><title>C ABI</title></head><body><h1>Hello from C</h1></body></html>");
    if (r != HB_OK) { fprintf(stderr, "load: %s\n", hb_result_string(r)); return 3; }
    r = hb_view_save_png(view, "hb_core_demo_c.png");
    printf("HB Core %s (ABI %d): save_png -> %s\n", hb_version_string(), hb_abi_version(), hb_result_string(r));
    hb_view_destroy(view);
    hb_core_destroy(core);
    return r == HB_OK ? 0 : 4;
}
