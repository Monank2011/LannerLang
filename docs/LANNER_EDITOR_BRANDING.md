# Lanner editor branding

Lanner source files use the `.st` extension. The official Lanner logo is packaged as a transparent PNG and wired into the bundled VS Code language contribution.

## VS Code

`tools/lanner-vscode/package.json` contributes the `lanner` language identifier for `.st` files and supplies the Lanner logo as the language icon. The same directory contributes a `Lanner File Icons` file-icon theme that maps both the `.st` extension and the `lanner` language ID to the logo.

To use the dedicated icon theme in VS Code, install the extension folder/package and select `Preferences: File Icon Theme` -> `Lanner File Icons` when needed. VS Code file icon themes can associate icons with file extensions and language IDs, while language contributions can also provide a default language icon. See the official VS Code file-icon theme and contribution-point documentation for the platform behavior.
