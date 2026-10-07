# Gambatte native PS5 — Game Boy / Game Boy Color

Pin exacto:

`libretro/gambatte-libretro@d9d6cd06382d1ced30de34d56d3609452323dab1`

El core Gambatte se compila como archivo estático y se enlaza directamente con el host nativo PS5 del repositorio. RetroArch no es una dependencia de ejecución.

Se generan dos entrypoints/títulos independientes sobre el mismo motor:

- Game Boy (`game.gb`)
- Game Boy Color (`game.gbc`)

Ruta: Gambatte -> libretro estático -> corehost -> ps5rt VideoOut/AudioOut/DualSense/VFS -> PS5.

El build obtiene el objeto Git exacto y verifica el SHA antes de compilar, desactiva el serial por red para este primer gate y ajusta el código host a Zen 2.

Un cross-build verde no implica todavía compatibilidad validada en consola física. Boot, contenido, SRAM, temporización y mando siguen siendo gates de hardware.
