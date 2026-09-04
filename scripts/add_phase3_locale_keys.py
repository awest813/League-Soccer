import io

BLOCKS = {
"en": """
league_lineup_hint=Click a player, then another, to swap their lineup positions.
league_lineup_bench=bench
league_lineup_selected=Selected: {0}. Pick a player to swap with.
league_lineup_gk_rule=Goalkeepers can only swap with goalkeepers.
league_lineup_swap_prompt=Swap these two players' lineup positions?
league_lineup_swapped=Lineup updated.
league_formation_current=Current: {0}
league_formation_factory=Restore club default
league_tactics_save=Save Tactics
league_tactics_saved=Tactics saved.
league_train_hint=Select a player above, then a training focus below.
league_train_accel=Acceleration
league_train_stamina=Stamina
league_train_dribble=Dribbling
league_train_shot=Shooting
league_train_select_first=Select a player first.
league_train_maxed=That attribute is already at its peak.
league_train_applied={0}: {1} -> {2}
league_release_hint=Use the button below to release this player.
league_release=Release
league_release_confirm=Release {0}? The player will leave the club.
league_release_too_small=Squad too small to release players (minimum {0}).
league_sign_hint=Use the button below to sign this player.
league_sign=Sign
league_sign_too_big=Squad is full (maximum {0}).
""",
"de": """
league_lineup_hint=Klicke einen Spieler an, dann einen zweiten, um ihre Positionen in der Aufstellung zu tauschen.
league_lineup_bench=Bank
league_lineup_selected=Ausgewählt: {0}. Wähle einen Spieler zum Tauschen.
league_lineup_gk_rule=Torhüter können nur mit Torhütern tauschen.
league_lineup_swap_prompt=Die Aufstellungspositionen dieser Spieler tauschen?
league_lineup_swapped=Aufstellung aktualisiert.
league_formation_current=Aktuell: {0}
league_formation_factory=Vereins-Standard wiederherstellen
league_tactics_save=Taktik sichern
league_tactics_saved=Taktik gesichert.
league_train_hint=Oben einen Spieler wählen, dann einen Trainingsschwerpunkt.
league_train_accel=Beschleunigung
league_train_stamina=Kondition
league_train_dribble=Dribbling
league_train_shot=Abschluss
league_train_select_first=Erst einen Spieler auswählen.
league_train_maxed=Dieser Wert ist bereits am Maximum.
league_train_applied={0}: {1} -> {2}
league_release_hint=Mit der Taste unten kannst du diesen Spieler entlassen.
league_release=Entlassen
league_release_confirm={0} entlassen? Der Spieler verlässt den Verein.
league_release_too_small=Squad zu klein (Minimum {0}).
league_sign_hint=Mit der Taste unten kannst du diesen Spieler verpflichten.
league_sign=Verpflichten
league_sign_too_big=Squad ist voll (Maximum {0}).
""",
"es": """
league_lineup_hint=Haz clic en un jugador y luego en otro para intercambiar sus posiciones.
league_lineup_bench=banco
league_lineup_selected=Seleccionado: {0}. Elige un jugador para intercambiar.
league_lineup_gk_rule=Los porteros solo pueden intercambiar con porteros.
league_lineup_swap_prompt=¿Intercambiar las posiciones de estos dos jugadores?
league_lineup_swapped=Alineación actualizada.
league_formation_current=Actual: {0}
league_formation_factory=Restaurar por defecto del club
league_tactics_save=Guardar táctica
league_tactics_saved=Táctica guardada.
league_train_hint=Selecciona un jugador arriba y luego un enfoque de entrenamiento.
league_train_accel=Aceleración
league_train_stamina=Resistencia
league_train_dribble=Regate
league_train_shot=Remate
league_train_select_first=Primero selecciona un jugador.
league_train_max=Ese atributo ya está en su máximo.
league_train_maxed=Ese atributo ya está en su máximo.
league_train_applied={0}: {1} -> {2}
league_release_hint=Usa el botón de abajo para liberar a este jugador.
league_release=Liberar
league_release_confirm=¿Liberar a {0}? El jugador dejará el club.
league_release_too_small=Plantilla demasiado pequeña (mínimo {0}).
league_sign_hint=Usa el botón de abajo para fichar a este jugador.
league_sign=Fichar
league_sign_too_big=Plantilla completa (máximo {0}).
""",
"fr": """
league_lineup_hint=Cliquez sur un joueur puis sur un autre pour échanger leurs positions.
league_lineup_bench=banc
league_lineup_selected=Sélectionné : {0}. Choisissez un joueur à échanger.
league_lineup_gk_rule=Les gardiens ne peuvent être échangés qu'entre eux.
league_lineup_swap_prompt=Échanger les positions de ces deux joueurs ?
league_lineup_swapped=Composition mise à jour.
league_formation_current=Actuelle : {0}
league_formation_factory=Restaurer la formation du club
league_tactics_save=Sauver la tactique
league_tactics_saved=Tactique sauvée.
league_train_hint=Sélectionnez un joueur ci-dessus, puis un axe d'entraînement.
league_train_accel=Accélération
league_train_stamina=Endurance
league_train_dribble=Dribble
league_train_shot=Frappe
league_train_select_first=Sélectionnez d'abord un joueur.
league_train_maxed=Cette qualité est déjà au maximum.
league_train_applied={0} : {1} -> {2}
league_release_hint=Utilisez le bouton ci-dessous pour libérer ce joueur.
league_release=Libérer
league_release_confirm=Libérer {0} ? Le joueur quittera le club.
league_release_too_small=Effectif trop petit (minimum {0}).
league_sign_hint=Utilisez le bouton ci-dessous pour recruter ce joueur.
league_sign=Recruter
league_sign_too_big=Effectif complet (maximum {0}).
""",
"pt": """
league_lineup_hint=Clique num jogador e depois noutro para trocar as posições.
league_lineup_bench=banco
league_lineup_selected=Selecionado: {0}. Escolha um jogador para trocar.
league_lineup_gk_rule=Guarda-redes só podem trocar com guarda-redes.
league_lineup_swap_prompt=Trocar as posições destes dois jogadores?
league_lineup_swamped=Onze atualizado.
league_lineup_swapped=Onze atualizado.
league_formation_current=Atual: {0}
league_formation_factory=Restaurar predefinido do clube
league_tactics_save=Guardar tática
league_tactics_saved=Tática guardada.
league_train_hint=Selecione um jogador acima e depois um foco de treino.
league_train_accel=Aceleração
league_train_stamina=Resistência
league_train_dribble=Condução
league_train_shot=Remate
league_train_select_first=Primeiro selecione um jogador.
league_train_maxed=Esse atributo já está no máximo.
league_train_applied={0}: {1} -> {2}
league_release_hint=Use o botão abaixo para liberar este jogador.
league_release=Liberar
league_release_confirm=Liberar {0}? O jogador deixará o clube.
league_release_too_small=Plantel pequeno demais (mínimo {0}).
league_sign_hint=Use o botão abaixo para contratar este jogador.
league_sign=Contratar
league_sign_too_big=Plantel cheio (máximo {0}).
""",
}

for lang, block in BLOCKS.items():
    path = "data/locale/%s.ini" % lang
    with io.open(path, "r", encoding="utf-8", newline="") as f:
        content = f.read()
    existing = set()
    for line in content.splitlines():
        line = line.strip()
        if "=" in line and not line.startswith(("#", ";")):
            existing.add(line.split("=", 1)[0].strip())
    to_add = []
    for line in block.strip().splitlines():
        if "=" not in line:
            continue
        key = line.split("=", 1)[0].strip()
        if key and key not in existing:
            to_add.append(line)
    if not to_add:
        print("%s: nothing to add" % lang)
        continue
    if not content.endswith("\n"):
        content += "\r\n"
    content += "\r\n# League mode: team management\r\n"
    content += "\r\n".join(to_add) + "\r\n"
    with io.open(path, "w", encoding="utf-8", newline="") as f:
        f.write(content)
    print("%s: added %d keys" % (lang, len(to_add)))
