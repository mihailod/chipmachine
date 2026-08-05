# cocoplugin — Coconizer

**Coconizer**, a sample-based Acorn Archimedes music format (the same VIDC-era
family as Archimedes Tracker).

Extensions: `.coco`

## Engine

libxmp's Coconizer loader, compiled as a **minimal single-loader slice** rather
than as the full ~58-format libxmp — the same approach as
[musxplugin](../musxplugin/README.md), [mgtplugin](../mgtplugin/README.md) and
[fnkplugin](../fnkplugin/README.md).

Note: Release builds here do **not** define `NDEBUG`, so libxmp's asserts are
live and will `abort()` on malformed input rather than misbehave silently.
