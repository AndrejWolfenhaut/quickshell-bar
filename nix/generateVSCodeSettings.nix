{ pkgs, generationScript }: pkgs.writeShellApplication {
  runtimeInputs = [ pkgs.mktemp pkgs.jq ];

  name = "generate-vscode-settings";

  text = ''
    changeFile() {
      currFile="./.vscode/$1"
    }
    assign() {
      local tmp
      tmp="$(mktemp)"

      cat "$currFile" > "$tmp"
      jq --argjson value "$2" "$1"' = $value' "$tmp" > "$currFile"
      rm "$tmp"
    }
    append() {
      local tmp
      tmp="$(mktemp)"

      cat "$currFile" > "$tmp"
      jq --argjson value "$2" "$1"' += [$value]' "$tmp" > "$currFile"
      rm "$tmp"
    }

    if [ -d ./.vscode ]; then
      rm -rf ./.vscode
    fi

    mkdir ./.vscode

    for settingsTemplate in ${./vsCodeSettingsTemplate}/*; do
      settingsTemplateBasename="$(basename "$settingsTemplate")"
      settingsTemplateNewBasename="''${settingsTemplateBasename%_template.*}.''${settingsTemplateBasename##*.}"

      cp "$settingsTemplate" "./.vscode/$settingsTemplateNewBasename"

      chmod 644 "./.vscode/$settingsTemplateNewBasename"
    done

    ${generationScript}
  '';
}
