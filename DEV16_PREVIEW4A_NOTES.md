# dev.16-preview.4a — GPT28b stack safety

Hardware incident: selecting GPT28b on preview.4 rebooted ESP32-S3; no panic trace was captured. The two per-callback local geometry arrays (108 MycSeg + 108 MycRasterSeg) are a plausible stack-overflow cause, **not confirmed**.

Changes: allocate a persistent MycWorkspace once on the heap when GPT28b is first invoked. Reuse it on all subsequent frames; no heap allocations per frame. If the allocation fails, exit without rendering and do not retry the allocation every frame. Both geometrical arrays and iteration math are unchanged; 32-pixel row scratch remains on the stack. All other rendering functions unchanged from preview.4.

Hardware qualification and full Kagayaki/PlatformIO compile remain pending. Never deliberately retrigger preview.4 crash to obtain a log.
