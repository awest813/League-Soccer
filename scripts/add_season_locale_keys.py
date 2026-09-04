import io

BLOCKS = {
"de": """
league_back_hub=Zurück zur Liga-Zentrale
league_calendar_filter=Nach Liga filtern:
league_club_snapshot=Vereinsprofil
league_configured=Konfiguriert
league_guidance_body=Squad-Aufstellung prüfen, Formation einstellen, dann die Taktik vor dem Anpfiff verfeinern.
league_needs_setup=Einrichtung nötig
league_next_fixture=Nächstes Spiel
league_season_pulse=Saison-Lage
league_squad_body={0} Spieler\nDurchschnitt: {1}\nAlter: {2} bis {3}
league_structure_body=Formation: {0}\nTaktik: {1}
league_calendar=Kalender / Spielplan
league_play_next_match=Nächstes Spiel spielen
league_advance_matchday=Zum nächsten Spieltag
league_no_fixture=Kein Spiel angesetzt
league_manager_date=Manager: {0} | Datum: {1}
league_players_registered={0} registrierte Spieler
league_season_pulse_body={0} Spiele, {1} Punkte
league_unread_inbox=Ungelesen: {0}
league_no_standings_data=Noch keine Tabellendaten
league_calendar_header=Datum            | Heim              | Gast              | Liga
league_simulate_prompt=Dieses Spiel simulieren und ein Ergebnis erzeugen?
league_simulate=Simulieren
league_matchday_title=Spieltag-Ergebnisse
league_matchday_none=Noch kein Spieltag gespielt
league_matchday_empty=Keine Ergebnisse für diesen Spieltag.
league_matchday_continue=Weiter
league_prematch_title=SPIELTAG
league_prematch_kickoff=Anpfiff
league_prematch_simulate=Spiel simulieren
league_prematch_home=Heimspiel
league_prematch_away=Auswärtsspiel bei {0}
league_prematch_no_fixture=Kein Spiel angesetzt.
league_table_title=Liga-Tabelle
league_table_header=Team                          | Sp | S  | U  | N  | T  | GT | TD | Pkt | Form
league_form=Form
league_stats_total=Gespielte Spiele: {0}
league_stats_high_header=--- Torreichste Spiele ---
league_stats_none=Noch keine Spiele. Simuliere Spiele im Kalender.
league_inbox_unread={0} ungelesene Nachricht(en)
league_inbox_empty=Noch keine Nachrichten. Nachrichten erscheinen mit gespielten Spielen.
league_inbox_close=Schließen
league_inbox_from=Von: {0}
league_inbox_date=Datum: {0}
league_inbox_no_content=Kein Inhalt.
league_newdb_select=Grunddatenbank wählen
league_currency_select=Währung wählen
league_savegame_name=Spielstand-Name
league_select_team=Verein wählen
league_initial_difficulty=Anfangsschwierigkeit
league_newdb_help1=Die Grunddatenbank wird in ein neues Verzeichnis kopiert, das als 'Spielstand' deiner Liga dient. Deine Liga basiert auf dieser Datenbank; spätere Änderungen an der Grunddatenbank beeinflussen deinen Spielstand nicht (und umgekehrt).
league_newdb_help2=Grunddatenbanken liegen im Unterverzeichnis 'databases' deiner League-Soccer-Installation. Gespeicherte Ligen und Pokale liegen im Verzeichnis 'saves'.
league_select_source_db=Quell-Datenbank wählen
""",
"es": """
league_back_hub=Volver al centro del club
league_calendar_filter=Filtrar por liga:
league_club_snapshot=Perfil del club
league_configured=Configurado
league_guidance_body=Revisa la alineación, ajusta la formación y afina la táctica antes del saque inicial.
league_needs_setup=Necesita ajustes
league_next_fixture=Próximo partido
league_season_pulse=Pulso de la temporada
league_squad_body={0} jugadores\nMedia: {1}\nEdad: {2} a {3}
league_structure_body=Formación: {0}\nTáctica: {1}
league_calendar=Calendario / Partidos
league_play_next_match=Jugar próximo partido
league_advance_matchday=Avanzar a la siguiente jornada
league_no_fixture=No hay partido programado
league_manager_date=Manager: {0} | Fecha: {1}
league_players_registered={0} jugadores registrados
league_season_pulse_body={0} partidos jugados, {1} puntos
league_unread_inbox=No leídos: {0}
league_no_standings_data=Aún sin datos de clasificación
league_calendar_header=Fecha            | Local              | Visitante          | Liga
league_simulate_prompt=¿Simular este partido para generar un resultado?
league_simulate=Simular
league_matchday_title=Resultados de la jornada
league_matchday_none=Todavía no se ha jugado ninguna jornada
league_matchday_empty=Sin resultados para esta jornada.
league_matchday_continue=Continuar
league_prematch_title=JORNADA
league_prematch_kickoff=Saque inicial
league_prematch_simulate=Simular partido
league_prematch_home=Juegas en casa
league_prematch_away=Juegas fuera contra {0}
league_prematch_no_fixture=No hay partido programado.
league_table_title=Tabla de la liga
league_table_header=Equipo                        | PJ | G  | E  | P  | GF | GC | DG | Pts | Forma
league_form=Forma
league_stats_total=Partidos jugados: {0}
league_stats_high_header=--- Partidos con más goles ---
league_stats_none=Todavía no hay partidos. Simula partidos desde el Calendario.
league_inbox_unread={0} mensaje(s) sin leer
league_inbox_empty=Aún no hay mensajes. Aparecerán al jugar partidos.
league_inbox_close=Cerrar
league_inbox_from=De: {0}
league_inbox_date=Fecha: {0}
league_inbox_no_content=Sin contenido.
league_newdb_select=Selecciona base de datos
league_currency_select=Selecciona moneda
league_savegame_name=Nombre del guardado
league_select_team=Selecciona tu equipo
league_initial_difficulty=Dificultad inicial
league_newdb_help1=La base de datos base se copiará a un nuevo directorio que servirá como 'partida guardada' de tu liga. Tu liga se basa en esta base de datos; los cambios posteriores no afectarán a tu guardado (ni al revés).
league_newdb_help2=Las bases de datos se guardan en el subdirectorio 'databases' de tu instalación de League Soccer. Las ligas y copas guardadas están en el directorio 'saves'.
league_select_source_db=Selecciona base de datos origen
""",
"fr": """
league_back_hub=Retour au hub de la ligue
league_calendar_filter=Filtrer par championnat :
league_club_snapshot=Profil du club
league_configured=Configuré
league_guidance_body=Vérifiez la composition, ajustez la formation, puis affinez la tactique avant le coup d'envoi.
league_needs_setup=À configurer
league_next_fixture=Prochain match
league_season_pulse=Pouls de la saison
league_squad_body={0} joueurs\\nMoyenne : {1}\\nÂge : {2} à {3}
league_structure_body=Formation : {0}\\nTactique : {1}
league_calendar=Calendrier / Matchs
league_play_next_match=Jouer le prochain match
league_advance_matchday=Journée suivante
league_no_fixture=Aucun match programmé
league_manager_date=Manager : {0} | Date : {1}
league_players_registered={0} joueurs enregistrés
league_season_pulse_body={0} matchs joués, {1} points
league_unread_inbox=Non lus : {0}
league_no_standings_data=Pas encore de classement
league_calendar_header=Date             | Domicile           | Extérieur          | Championnat
league_simulate_prompt=Simuler ce match pour générer un résultat ?
league_simulate=Simuler
league_matchday_title=Résultats de la journée
league_matchday_none=Aucune journée jouée pour le moment
league_matchday_empty=Aucun résultat pour cette journée.
league_matchday_continue=Continuer
league_prematch_title=JOURNÉE
league_prematch_kickoff=Coup d'envoi
league_prematch_simulate=Simuler le match
league_prematch_home=Vous jouez à domicile
league_prematch_away=Vous jouez à {0}
league_prematch_no_fixture=Aucun match programmé.
league_table_title=Classement
league_table_header=Équipe                        | J  | V  | N  | D  | BP | BC | DB | Pts | Forme
league_form=Forme
league_stats_total=Matchs joués : {0}
league_stats_high_header=--- Matchs les plus prolifiques ---
league_stats_none=Aucun match joué. Simulez des matchs depuis le calendrier.
league_inbox_unread={0} message(s) non lu(s)
league_inbox_empty=Aucun message pour l'instant. Les messages apparaîtront en jouant des matchs.
league_inbox_close=Fermer
league_inbox_from=De : {0}
league_inbox_date=Date : {0}
league_inbox_no_content=Aucun contenu.
league_newdb_select=Choisir la base de données
league_currency_select=Choisir la devise
league_savegame_name=Nom de la sauvegarde
league_select_team=Choisissez votre équipe
league_initial_difficulty=Difficulté initiale
league_newdb_help1=La base de données source sera copiée dans un nouveau répertoire servant de 'sauvegarde' à votre ligue. Votre ligue est basée sur cette base ; toute modification ultérieure de la base source n'affectera pas votre sauvegarde (ni l'inverse).
league_newdb_help2=Les bases de données sources sont stockées dans le sous-répertoire 'databases' de votre installation League Soccer. Les ligues et coupes sauvegardées sont dans le répertoire 'saves'.
league_select_source_db=Choisir la base source
""",
"pt": """
league_back_hub=Voltar ao hub da liga
league_calendar_filter=Filtrar por liga:
league_club_snapshot=Perfil do clube
league_configured=Configurado
league_guidance_body=Verifique a escalação, ajuste a formação e afine a tática antes do pontapé inicial.
league_needs_setup=Precisa de configuração
league_next_fixture=Próximo jogo
league_season_pulse=Pulso da temporada
league_squad_body={0} jogadores\\nMédia: {1}\\nIdade: {2} a {3}
league_structure_body=Formação: {0}\\nTática: {1}
league_calendar=Calendário / Jogos
league_play_next_match=Jogar próximo jogo
league_advance_matchday=Avançar para a próxima jornada
league_no_fixture=Sem jogo agendado
league_manager_date=Manager: {0} | Data: {1}
league_players_registered={0} jogadores registados
league_season_pulse_body={0} jogos disputados, {1} pontos
league_unread_inbox=Não lidas: {0}
league_no_standings_data=Ainda sem dados de classificação
league_calendar_header=Data             | Casa               | Visitante          | Liga
league_simulate_prompt=Simular este jogo para gerar um resultado?
league_simulate=Simular
league_matchday_title=Resultados da jornada
league_matchday_none=Nenhuma jornada disputada ainda
league_matchday_empty=Sem resultados para esta jornada.
league_matchday_continue=Continuar
league_prematch_title=JORNADA
league_prematch_kickoff=Pontapé inicial
league_prematch_simulate=Simular jogo
league_prematch_home=Joga em casa
league_prematch_away=Joga fora contra {0}
league_prematch_no_fixture=Sem jogo agendado.
league_table_title=Classificação da liga
league_table_header=Equipa                        | J  | V  | E  | D  | GM | GS | DG | Pts | Forma
league_form=Forma
league_stats_total=Jogos disputados: {0}
league_stats_high_header=--- Jogos com mais golos ---
league_stats_none=Ainda sem jogos. Simule jogos no Calendário.
league_inbox_unread={0} mensagem(ns) não lida(s)
league_inbox_empty=Ainda sem mensagens. As mensagens aparecem ao jogar.
league_inbox_close=Fechar
league_inbox_from=De: {0}
league_inbox_date=Data: {0}
league_inbox_no_content=Sem conteúdo.
league_newdb_select=Selecionar base de dados
league_currency_select=Selecionar moeda
league_savegame_name=Nome do savegame
league_select_team=Selecione o seu clube
league_initial_difficulty=Dificuldade inicial
league_newdb_help1=A base de dados base será copiada para um novo diretório que servirá de 'save' para a sua liga. A sua liga baseia-se nesta base de dados; alterações posteriores à base original não afetam o seu save (e vice-versa).
league_newdb_help2=As bases de dados estão no subdiretório 'databases' da sua instalação do League Soccer. Ligas e taças guardadas ficam no diretório 'saves'.
league_select_source_db=Selecionar base de dados de origem
""",
}

for lang, block in BLOCKS.items():
    path = "data/locale/%s.ini" % lang
    with io.open(path, "r", encoding="utf-8", newline="") as f:
        content = f.read()
    existing = set()
    for line in content.splitlines():
        line = line.strip()
        if "=" in line and not line.startswith("#") and not line.startswith(";"):
            existing.add(line.split("=", 1)[0].strip())
    to_add = []
    for line in block.strip().splitlines():
        key = line.split("=", 1)[0].strip()
        if key and key not in existing:
            to_add.append(line)
    if not to_add:
        print("%s: nothing to add" % lang)
        continue
    section = "\r\n# League mode: season UI\r\n".encode() if isinstance("", bytes) else "# League mode: season UI\r\n"
    if not content.endswith("\n"):
        content += "\r\n"
    content += "\r\n# League mode: season UI\r\n"
    content += "\r\n".join(to_add) + "\r\n"
    with io.open(path, "w", encoding="utf-8", newline="") as f:
        f.write(content)
    print("%s: added %d keys" % (lang, len(to_add)))
