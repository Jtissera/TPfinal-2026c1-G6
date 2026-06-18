#include "Game.h"
#include "sdl/ECS/Components.h"
#include "sdl/Map.h"
#include "sdl/RenderContext.h"
#include "sdl/TextureManager.h"
#include "sdl/UpdateContext.h"
#include <iostream>
#include <sstream>
#include <unordered_set>

#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"

Game::Game() {}

void Game::init(SDL_Window *existingWindow, SDL_Renderer *existingRenderer,
                Queue<std::shared_ptr<const Message>> &sendQ,
                Queue<std::shared_ptr<const Message>> &receiveQ,
                const PlayerDto &pDto)
{

  this->sendQueue = &sendQ;
  this->receiveQueue = &receiveQ;
  this->playerDto = pDto;
  this->playerState = toPlayerViewState(this->playerDto);

  this->window = existingWindow;
  this->renderer = existingRenderer;

  if (this->window == nullptr || this->renderer == nullptr)
  {
    std::cerr << "[Game::init] window o renderer inválidos" << std::endl;
    isRunning = false;
    return;
  }

  textureManager = std::make_unique<TextureManager>(renderer);
  assets =
      std::make_unique<AssetManager>(&manager, *sendQueue, *textureManager);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  isRunning = true;

  loadAssets();

  try
  {
    itemCatalog.loadFromJson("assets/sprites/items/items.json");
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error cargando catálogo de ítems: " << e.what() << std::endl;
    isRunning = false;
    return;
  }

  player = assets->CreatePlayer(playerDto);

  clientWorld = std::make_unique<ClientGameWorld>(
      static_cast<uint32_t>(playerDto.playerID), player, *assets);

  refreshPlayerEquipmentVisuals();

  map = new Map(manager, *assets, "terrain", 3, 32);
  map->LoadMap("assets/sprites/MapAssets/map.argmap");
}

void Game::handleEvents()
{
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_QUIT)
    {
      isRunning = false;
    }

    if (event.type == SDL_KEYDOWN && event.key.repeat != 0)
    {
      event.type = SDL_USEREVENT;
    }
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
    {
      handleCheatKeys();
    }

    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT)
    {

      if (isLocalPlayerDead())
      {
        continue;
      }

      const int mouseX = event.button.x;
      const int mouseY = event.button.y;

      const int equipmentSlot = getEquipmentSlotIndexAt(mouseX, mouseY);

      if (equipmentSlot != -1)
      {
        handleEquipmentSlotClick(equipmentSlot);
        return;
      }

      const int inventorySlot = getInventorySlotIndexAt(mouseX, mouseY);

      if (inventorySlot != -1)
      {
        handleInventorySlotClick(inventorySlot);
        return;
      }
      const ItemView *equippedWeapon = nullptr;
      if (equipmentState.weapon.has_value())
      {
        equippedWeapon = &equipmentState.weapon.value();
      }
      std::vector<GroundPickupTarget> pickupTargets;

      for (auto &[instanceId, entity] : groundItems)
      {
        if (entity == nullptr) continue;
        pickupTargets.push_back(GroundPickupTarget{instanceId, false, entity});
      }

      for (auto &[instanceId, entity] : groundGold)
      {
        if (entity == nullptr) continue;
        pickupTargets.push_back(GroundPickupTarget{instanceId, true, entity});
      }

      if (pickUpSystem.handleMouseClick(mouseX, mouseY, pickupTargets, sendQueue))
      {
        return;
      }

      std::vector<AttackTarget> attackTargets;

      for (auto &[enemyId, enemyEntity] : enemies)
      {
        if (enemyEntity == nullptr)
        {
          continue;
        }

        attackTargets.push_back(AttackTarget{enemyId, enemyEntity});
      }

      // Jugadores remotos.
      if (clientWorld != nullptr)
      {
        clientWorld->appendRemoteAttackTargets(
            attackTargets, static_cast<uint8_t>(playerState.level));
      }

      attackSystem.handleMouseClick(mouseX, mouseY, camera, attackTargets,
                                    sendQueue, player, equippedWeapon);
    }
  }
}

void Game::update()
{

  std::shared_ptr<const Message> msg;
  try
  {
    while (receiveQueue->try_pop(msg))
    {
      processServerMessage(*msg);
    }
  }
  catch (const ClosedQueue &)
  {
    std::cerr << "[Game] receiveQueue cerrada, el server se desconectó."
              << std::endl;
    isRunning = false;
    return;
  }
  Vector2D playerPos = player->getComponent<TransformComponent>().position;
  camera.x = static_cast<int>(playerPos.x) - 450;
  camera.y = static_cast<int>(playerPos.y) - 343;
  if (camera.x < 0)
    camera.x = 0;
  if (camera.y < 0)
    camera.y = 0;
  if (camera.x > 100 * 96 - 900)
    camera.x = 100 * 96 - 900;
  if (camera.y > 100 * 96 - 687)
    camera.y = 100 * 96 - 687;

  // PERF: detecta si la cámara se movió este frame.
  // TileComponent::update() usa este flag para saltear 300 recálculos
  // cuando el jugador está quieto.
  const bool cameraMoved =
      (camera.x != prevCamera.x || camera.y != prevCamera.y);
  prevCamera = camera;

  UpdateContext updateContext{SDL_GetKeyboardState(nullptr), sendQueue, camera,
                              cameraMoved};
  manager.refresh();
  manager.update(updateContext);

  attackSystem.update();
  if (isLocalPlayerDead())
  {
    applyLocalPlayerGhostState();
  }
  else
  {
    if (hasReceivedValidPlayerStats && playerState.hp <= 0)
    {
      applyLocalPlayerGhostState();
    }
  }
}

void Game::render()
{

  // Limpia la pantalla antes de dibujar el nuevo frame.
  SDL_RenderClear(renderer);

  // Limita el dibujado al área del mapa, para que no invada el HUD.
  SDL_Rect mapArea = {0, 33, 900, 687};
  SDL_RenderSetClipRect(renderer, &mapArea);

  RenderContext renderContext{renderer, camera, mapArea, *textureManager, 133};

  for (auto &t : manager.getGroup(groupMap))
    t->draw(renderContext);

  for (auto &p : manager.getGroup(groupPlayers))
  {
    drawEquippedEntity(p, renderContext);
  }

  for (const auto &[enemyId, enemy] : enemies)
  {
    if (enemy == nullptr || attackSystem.isEnemyDead(enemyId))
      continue;
    enemy->draw(renderContext);
  }

  for (auto &t : manager.getGroup(groupMapTop))
    t->draw(renderContext);

  for (auto &n : manager.getGroup(groupNPC))
  {
    n->draw(renderContext);
  }
  for (auto&item:manager.getGroup(groupItems)) {
    item->draw(renderContext);
  }

  renderEnemyHealthBars();

  attackSystem.render(renderer, *assets, camera);
  SDL_RenderSetClipRect(renderer, nullptr);

  // PERF: statusMessage usa textura cacheada (creada en showStatusMessage).
  // Solo SDL_SetTextureAlphaMod() por frame para el fade — sin alloc.
  if (!statusMessage.empty() && statusMessageTexture != nullptr)
  {
    const Uint32 elapsed = SDL_GetTicks() - statusMessageTimer;
    if (elapsed < STATUS_MESSAGE_DURATION_MS)
    {
      Uint8 alpha = 255;
      const Uint32 fadeStart = STATUS_MESSAGE_DURATION_MS - 500;
      if (elapsed > fadeStart)
      {
        alpha = static_cast<Uint8>(
            255 * (1.0f - static_cast<float>(elapsed - fadeStart) / 500.0f));
      }
      SDL_SetTextureAlphaMod(statusMessageTexture, alpha);
      SDL_Rect dest = {(900 - statusMessageTexW) / 2, 350, statusMessageTexW,
                       statusMessageTexH};
      SDL_RenderCopy(renderer, statusMessageTexture, nullptr, &dest);
    }
    else
    {
      statusMessage.clear();
      SDL_DestroyTexture(statusMessageTexture);
      statusMessageTexture = nullptr;
    }
  }

  renderHUD();
  SDL_RenderPresent(renderer);
}

void Game::clean()
{
  clearTextCache();

  // PERF: destruir textura cacheada del statusMessage si quedó activa.
  if (statusMessageTexture != nullptr)
  {
    SDL_DestroyTexture(statusMessageTexture);
    statusMessageTexture = nullptr;
  }

  assets.reset();
  textureManager.reset();

  if (map != nullptr)
  {
    delete map;
    map = nullptr;
  }

  // No destruimos renderer ni window acá.
  // Son propiedad de main().
  renderer = nullptr;
  window = nullptr;

  std::cout << "Game cleaned." << std::endl;
}

void Game::showStatusMessage(const std::string &msg)
{
  // PERF: destruir textura anterior si existe.
  if (statusMessageTexture != nullptr)
  {
    SDL_DestroyTexture(statusMessageTexture);
    statusMessageTexture = nullptr;
  }

  statusMessage = msg;
  statusMessageTimer = SDL_GetTicks();

  // Crear la textura una sola vez — render() solo aplica alpha cada frame.
  if (statusFont != nullptr && !msg.empty())
  {
    SDL_Color red = {255, 50, 50, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(statusFont, msg.c_str(), red);
    if (surf != nullptr)
    {
      statusMessageTexture = SDL_CreateTextureFromSurface(renderer, surf);
      if (statusMessageTexture != nullptr)
      {
        SDL_SetTextureBlendMode(statusMessageTexture, SDL_BLENDMODE_BLEND);
        statusMessageTexW = surf->w;
        statusMessageTexH = surf->h;
      }
      SDL_FreeSurface(surf);
    }
  }
}

// ---------------------------------------------------------------------------
// Cheats — combinaciones Ctrl+tecla, sin repeat
// ---------------------------------------------------------------------------
void Game::handleCheatKeys()
{
  const Uint8 *keys = SDL_GetKeyboardState(nullptr);
  const bool ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
  if (!ctrl)
    return;

  switch (event.key.keysym.sym)
  {

  // Ctrl+h — God mode (vida infinita)
  case SDLK_h:
    cheatGodMode = !cheatGodMode;
    if (cheatGodMode)
      cheatInfMana = false;
    showStatusMessage(cheatGodMode ? "[CHEAT] God mode ON"
                                   : "[CHEAT] God mode OFF");
    // Notificar al servidor: toggle HP infinito
    sendQueue->try_push(
        std::make_shared<const CheatMessage>(CheatType::INFINITE_HP));
    break;

  // Ctrl+m — Mana infinito (solo mana)
  case SDLK_m:
    if (!cheatGodMode)
    {
      cheatInfMana = !cheatInfMana;
      showStatusMessage(cheatInfMana ? "[CHEAT] Mana infinito ON"
                                     : "[CHEAT] Mana infinito OFF");
      // Notificar al servidor
      sendQueue->try_push(
          std::make_shared<const CheatMessage>(CheatType::INFINITE_MANA));
    }
    break;

  // Ctrl+k — Morir instantáneamente (servidor aplica la muerte)
  case SDLK_k:
    if (!playerState.isDead)
    {
      showStatusMessage("[CHEAT] Muriendo...");
      cheatGodMode = false;
      cheatInfMana = false;
      sendQueue->try_push(std::make_shared<const CheatMessage>(CheatType::DIE));
    }
    break;

  // Ctrl+l — Subir nivel local (solo visual, para testear HUD)
  case SDLK_l:
    showStatusMessage("[CHEAT]Solicitando Exp... ");
    sendQueue->try_push(
        std::make_shared<const CheatMessage>(CheatType::LEVEL_UP));
    break;

  // Ctrl+r — Resucitar (ya conectado al servidor)
  case SDLK_r:
    if (isLocalPlayerDead())
    {
      sendQueue->try_push(std::make_shared<const ResurrectMessage>());
      showStatusMessage("Solicitando resurrección...");
    }
    break;
  // Ctrl+g — añade oro (ya conectado al servidor)
  case SDLK_g:
    showStatusMessage("[CHEAT] Solicitando oro...");
    sendQueue->try_push(
        std::make_shared<const CheatMessage>(CheatType::ADD_GOLD));
    break;

  default:
    break;
  }
}

bool Game::running() const { return isRunning; }

void Game::renderHUD()
{
  // === FONDOS ===
  SDL_Texture *texTop = assets->GetTexture("hud_top");
  SDL_Texture *texLogo = assets->GetTexture("hud_logo");
  SDL_Texture *texChat = assets->GetTexture("hud_chat");
  SDL_Texture *texPjInfo = assets->GetTexture("hud_pj_info");
  SDL_Texture *texInv = assets->GetTexture("hud_inv");
  SDL_Texture *texStats = assets->GetTexture("hud_stats");

  SDL_Rect rTop = {0, 0, 1280, 33};
  SDL_Rect rLogo = {5, 0, 177, 33};
  SDL_Rect rChat = {0, 33, 900, 100};
  SDL_Rect rPjInfo = {900, 33, 380, 100};
  SDL_Rect rInv = {900, 133, 380, 442};
  SDL_Rect rStats = {900, 575, 380, 145};

  if (texTop)
    SDL_RenderCopy(renderer, texTop, nullptr, &rTop);
  if (texLogo)
    SDL_RenderCopy(renderer, texLogo, nullptr, &rLogo);
  if (texChat)
    SDL_RenderCopy(renderer, texChat, nullptr, &rChat);
  if (texPjInfo)
    SDL_RenderCopy(renderer, texPjInfo, nullptr, &rPjInfo);
  if (texInv)
    SDL_RenderCopy(renderer, texInv, nullptr, &rInv);
  if (texStats)
    SDL_RenderCopy(renderer, texStats, nullptr, &rStats);

  // === BORDES ===
  SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
  SDL_RenderDrawLine(renderer, 0, 33, 1280, 33);
  SDL_RenderDrawLine(renderer, 0, 34, 1280, 34);
  SDL_RenderDrawLine(renderer, 0, 133, 900, 133);
  SDL_RenderDrawLine(renderer, 0, 134, 900, 134);
  SDL_RenderDrawLine(renderer, 900, 33, 900, 720);
  SDL_RenderDrawLine(renderer, 901, 33, 901, 720);
  SDL_RenderDrawLine(renderer, 900, 133, 1280, 133);
  SDL_RenderDrawLine(renderer, 900, 134, 1280, 134);
  SDL_RenderDrawLine(renderer, 900, 575, 1280, 575);
  SDL_RenderDrawLine(renderer, 900, 576, 1280, 576);

  // === FUENTES Y COLORES ===
  TTF_Font *fontBold = assets->GetFont("ao_bold");
  TTF_Font *fontRegular = assets->GetFont("ao_regular");

  if (fontBold == nullptr || fontRegular == nullptr)
  {
    return;
  }

  SDL_Color white = {255, 255, 255, 255};
  SDL_Color yellow = {255, 215, 0, 255};

  // === HELPERS DE TEXTO CACHEADO ===
  auto drawTextCentered = [&](const std::string &key, const std::string &text,
                              TTF_Font *font, int x, int y, int w, int h,
                              SDL_Color color)
  {
    int textW = 0;
    int textH = 0;

    SDL_Texture *texture =
        getOrCreateTextTexture(key, text, font, color, textW, textH);

    if (texture == nullptr)
    {
      return;
    }

    SDL_Rect dest = {x + (w - textW) / 2, y + (h - textH) / 2, textW, textH};

    SDL_RenderCopy(renderer, texture, nullptr, &dest);
  };

  auto drawTextAt = [&](const std::string &key, const std::string &text,
                        TTF_Font *font, int x, int y, SDL_Color color)
  {
    int textW = 0;
    int textH = 0;

    SDL_Texture *texture =
        getOrCreateTextTexture(key, text, font, color, textW, textH);

    if (texture == nullptr)
    {
      return;
    }

    SDL_Rect dest = {x, y, textW, textH};
    SDL_RenderCopy(renderer, texture, nullptr, &dest);
  };

  // === CAJA DE NIVEL ===
  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
  SDL_Rect nivelBox = {908, 38, 50, 50};
  SDL_RenderFillRect(renderer, &nivelBox);

  SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
  SDL_RenderDrawRect(renderer, &nivelBox);

  drawTextCentered("hud_level", std::to_string(playerState.level), fontBold,
                   908, 38, 50, 50, yellow);

  // === NOMBRE Y CLASE ===
  drawTextAt("hud_name", playerState.name, fontBold, 968, 45, yellow);

  drawTextAt("hud_class", playerClassToString(playerState.playerClass),
             fontRegular, 968, 75, white);

  // === EQUIPAMIENTO ===
  drawTextCentered("hud_title_equipment", "Equipamiento", fontRegular, 900, 142,
                   380, 20, white);

  SDL_Texture *texFrame = assets->GetTexture("hud_frame");

  const int eqSlotSize = 58;
  const int eqGap = 12;
  const int eqY = 168;
  const int eqStartX = 956;

  std::string eqLabels[] = {"Arma", "Casco", "Armadura", "Escudo"};

  for (int i = 0; i < 4; i++)
  {
    SDL_Rect slot = {eqStartX + i * (eqSlotSize + eqGap), eqY, eqSlotSize,
                     eqSlotSize};

    if (texFrame != nullptr)
    {
      SDL_RenderCopy(renderer, texFrame, nullptr, &slot);
    }

    const ItemView *equippedItem = nullptr;

    if (i == 0 && equipmentState.weapon.has_value())
    {
      equippedItem = &equipmentState.weapon.value();
    }
    else if (i == 1 && equipmentState.helmet.has_value())
    {
      equippedItem = &equipmentState.helmet.value();
    }
    else if (i == 2 && equipmentState.armor.has_value())
    {
      equippedItem = &equipmentState.armor.value();
    }
    else if (i == 3 && equipmentState.shield.has_value())
    {
      equippedItem = &equipmentState.shield.value();
    }

    if (equippedItem != nullptr)
    {
      SDL_Texture *itemTexture = assets->GetTexture(equippedItem->textureId);

      if (itemTexture != nullptr)
      {
        SDL_Rect itemSrc = {equippedItem->iconSrcX, equippedItem->iconSrcY,
                            equippedItem->iconSrcW, equippedItem->iconSrcH};

        SDL_Rect itemDest = {slot.x + 7, slot.y + 7, slot.w - 14, slot.h - 14};

        SDL_RenderCopy(renderer, itemTexture, &itemSrc, &itemDest);
      }
    }

    drawTextCentered("hud_eq_label_" + std::to_string(i), eqLabels[i],
                     fontRegular, slot.x - 8, slot.y + eqSlotSize + 4,
                     eqSlotSize + 16, 14, white);
  }

  // === INVENTARIO ===
  const int inventoryTitleY = 255;

  drawTextCentered("hud_title_inventory", "Inventario", fontRegular, 900,
                   inventoryTitleY, 380, 20, white);

  const int invSlotSize = 44;
  const int invGapX = 8;
  const int invGapY = 7;

  const int invStartX = 964;
  const int invStartY = 280;
  const int invCols = 5;
  const int invRows = 4;

  for (int fila = 0; fila < invRows; fila++)
  {
    for (int col = 0; col < invCols; col++)
    {
      const int index = fila * invCols + col;

      SDL_Rect slot = {invStartX + col * (invSlotSize + invGapX),
                       invStartY + fila * (invSlotSize + invGapY), invSlotSize,
                       invSlotSize};

      SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
      SDL_RenderFillRect(renderer, &slot);

      SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
      SDL_RenderDrawRect(renderer, &slot);

      if (index < static_cast<int>(inventoryState.slots.size()) &&
          inventoryState.slots[index].has_value())
      {

        const ItemView &item = inventoryState.slots[index].value();
        SDL_Texture *itemTexture = assets->GetTexture(item.textureId);

        if (itemTexture != nullptr)
        {
          SDL_Rect itemDest = {slot.x + 5, slot.y + 5, slot.w - 10,
                               slot.h - 10};

          SDL_Rect itemSrc = {item.iconSrcX, item.iconSrcY, item.iconSrcW,
                              item.iconSrcH};

          SDL_RenderCopy(renderer, itemTexture, &itemSrc, &itemDest);
        }

        if (item.quantity > 1)
        {
          drawTextAt("hud_item_qty_" + std::to_string(index),
                     std::to_string(item.quantity), fontRegular,
                     slot.x + slot.w - 14, slot.y + slot.h - 16, white);
        }
      }
    }
  }

  // === BARRAS ===
  const int hpActual = playerState.hp;
  const int hpMax = playerState.maxHp;

  const int manaActual = playerState.mana;
  const int manaMax = playerState.maxMana;

  const int expActual = playerState.exp;
  const int expMax = playerState.expToNextLevel;

  auto drawBar = [&](const std::string &key, SDL_Texture *tex, int x, int y,
                     int w, int h, int actual, int max, TTF_Font *font)
  {
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);

    SDL_Rect bgRect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &bgRect);

    const int fillW = max > 0 ? (w * actual) / max : 0;

    if (tex != nullptr && fillW > 0)
    {
      SDL_Rect srcR = {0, 0, fillW, h};
      SDL_Rect fillR = {x, y, fillW, h};
      SDL_RenderCopy(renderer, tex, &srcR, &fillR);
    }

    const std::string text = std::to_string(actual) + "/" + std::to_string(max);

    drawTextCentered(key, text, font, x, y, w, h, white);
  };

  SDL_Texture *texVida = assets->GetTexture("barra_vida");
  SDL_Texture *texMana = assets->GetTexture("barra_mana");
  SDL_Texture *texExp = assets->GetTexture("barra_exp");

  // Experiencia
  drawTextCentered("hud_label_exp", "Experiencia", fontRegular, 910, 100, 350,
                   16, white);

  drawBar("hud_exp_bar_text", texExp, 910, 118, 350, 16, expActual, expMax,
          fontRegular);

  // Oro
  drawTextAt("hud_gold", "Oro: " + std::to_string(playerState.gold),
             fontRegular, 915, 585, yellow);

  // Vida
  const int statsX = 950;
  const int statsBarW = 260;
  const int statsBarH = 18;

  drawTextCentered("hud_label_hp", "Vida", fontRegular, statsX, 610, statsBarW,
                   18, white);

  drawBar("hud_hp_bar_text", texVida, statsX, 630, statsBarW, statsBarH,
          hpActual, hpMax, fontRegular);

  // Mana
  drawTextCentered("hud_label_mana", "Mana", fontRegular, statsX, 665,
                   statsBarW, 18, white);

  drawBar("hud_mana_bar_text", texMana, statsX, 685, statsBarW, statsBarH,
          manaActual, manaMax, fontRegular);
}
void Game::loadAssets()
{
  assets->LoadManifest("assets/manifest.json");

  // HUD - fondos locales del proyecto.
  assets->AddTexture("hud_top", "assets/sprites/ui/hud/main_top.png");
  assets->AddTexture("hud_chat", "assets/sprites/ui/hud/main_chat.png");
  assets->AddTexture("hud_pj_info", "assets/sprites/ui/hud/main_pj_info.png");
  assets->AddTexture("hud_inv", "assets/sprites/ui/hud/inventory_bg.png");
  assets->AddTexture("hud_stats", "assets/sprites/ui/hud/stats_bg.png");
  assets->AddTexture("hud_logo", "assets/sprites/ui/hud/ao20_logo_med.png");
  assets->AddTexture("hud_pergamino",
                     "assets/sprites/ui/hud/titulo_pergamino.png");
  assets->AddTexture("hud_frame", "assets/sprites/ui/hud/frame.png");

  // Fuentes locales del proyecto.
  assets->AddFont("ao_bold",
                  "assets/sprites/ui/fonts/Alegreya-Sans-AO-Bold.ttf", 18);
  assets->AddFont("ao_regular",
                  "assets/sprites/ui/fonts/Alegreya-Sans-AO-Regular.ttf", 14);
  assets->AddFont("cardo", "assets/sprites/ui/fonts/Cardo-Regular.ttf", 14);

  statusFont = assets->GetFont("ao_bold");

  if (!statusFont)
  {
    statusFont =
        TTF_OpenFont("assets/sprites/ui/fonts/Alegreya-Sans-AO-Bold.ttf", 24);
  }

  if (!statusFont)
  {
    statusFont = TTF_OpenFont("assets/sprites/MapAssets/arial.ttf", 24);
  }

  // Barras HUD.
  assets->AddTexture("barra_vida", "assets/sprites/ui/bars/es_barradevida.bmp");
  assets->AddTexture("barra_mana", "assets/sprites/ui/bars/es_barrademana.bmp");
  assets->AddTexture("barra_exp",
                     "assets/sprites/ui/bars/es_barraexperiencia.bmp");

  // Fuente fallback.
  assets->AddFont("arial", "assets/sprites/MapAssets/arial.ttf", 16);

  // Terreno.
  assets->AddTexture("tile_grass", "assets/sprites/MapAssets/tile_grass.png");
  assets->AddTexture("tile_water", "assets/sprites/MapAssets/tile_water.png");
  assets->AddTexture("tile_sand", "assets/sprites/MapAssets/tile_sand.png");
  assets->AddTexture("tile_tree", "assets/sprites/MapAssets/tree.png");
  assets->AddTexture("tile_tree2", "assets/sprites/MapAssets/tree2.png");
  assets->AddTexture("tile_tree3", "assets/sprites/MapAssets/tree3.png");
  assets->AddTexture("tile_stone", "assets/sprites/MapAssets/stone.png");
  assets->AddTexture("tile_stone2", "assets/sprites/MapAssets/stone2.png");
  assets->AddTexture("tile_stone3", "assets/sprites/MapAssets/stone3.png");
  assets->AddTexture("tile_cactus", "assets/sprites/MapAssets/cactus.png");
  assets->AddTexture("tile_cactus2", "assets/sprites/MapAssets/cactus2.png");
  assets->AddTexture("tile_cactus3",
                     "assets/sprites/MapAssets/desert_rock.png");
  assets->AddTexture("tile_dungeon_entrance",
                     "assets/sprites/MapAssets/dungeon_entrance.png");
  assets->AddTexture("tile_cavern_entrance",
                     "assets/sprites/MapAssets/cavern_entrance.png");
  assets->AddTexture("tile_city_floor", "assets/sprites/MapAssets/city_floor.png");
  assets->AddTexture("tile_house", "assets/sprites/MapAssets/house.png");
  assets->AddTexture("tile_church", "assets/sprites/MapAssets/church.png");
  assets->AddTexture("tile_mill", "assets/sprites/MapAssets/mill.png");
  assets->AddTexture("tile_cavern_floor", "assets/sprites/MapAssets/cavern_floor.png");
  assets->AddTexture("tile_cavern_vertical_wall", "assets/sprites/MapAssets/cavern_vertical_wall.png");
  assets->AddTexture("tile_cavern_horizontal_wall", "assets/sprites/MapAssets/cavern_horizontal_wall.png");
  assets->AddTexture("tile_dungeon_floor", "assets/sprites/MapAssets/dungeon_floor.png");
  assets->AddTexture("tile_exit", "assets/sprites/MapAssets/exit.png");

  assets->AddTexture("npc_priest", "assets/sprites/npcs/priest.png");
  assets->AddTexture("npc_shop", "assets/sprites/npcs/shop.png");
  assets->AddTexture("npc_bank", "assets/sprites/npcs/bank.png");
}

int Game::getInventorySlotIndexAt(int mouseX, int mouseY) const
{
  // deben ir los mismos valores que el inventario del renderhud.
  const int invSlotSize = 44;
  const int invGapX = 8;
  const int invGapY = 7;
  const int invStartX = 964;
  const int invStartY = 280;
  const int invCols = 5;
  const int invRows = 4;

  for (int fila = 0; fila < invRows; fila++)
  {
    for (int col = 0; col < invCols; col++)
    {
      SDL_Rect slot = {invStartX + col * (invSlotSize + invGapX),
                       invStartY + fila * (invSlotSize + invGapY), invSlotSize,
                       invSlotSize};

      const bool inside = mouseX >= slot.x && mouseX < slot.x + slot.w &&
                          mouseY >= slot.y && mouseY < slot.y + slot.h;

      if (inside)
      {
        return fila * invCols + col;
      }
    }
  }

  return -1;
}
void Game::handleInventorySlotClick(int slotIndex)
{
  // Valida que el índice sea válido.
  if (slotIndex < 0 ||
      slotIndex >= static_cast<int>(inventoryState.slots.size()))
  {
    return;
  }

  // Si el slot está vacío, no hacemos nada.
  if (!inventoryState.slots[slotIndex].has_value())
  {
    std::cout << "[INVENTORY] slot vacío: " << slotIndex << std::endl;
    return;
  }

  // Obtenemos el ítem del slot clickeado.
  const ItemView &item = inventoryState.slots[slotIndex].value();


  if (item.type == ClientItemType::HealthPotion ||
      item.type == ClientItemType::ManaPotion)
  {

    std::cout << "[INVENTORY] pedido usar poción item=" << item.itemName
              << " instanceId=" << item.instanceId << std::endl;

    sendQueue->try_push(
        std::make_shared<const UseItemMessage>(item.instanceId));
    return;
  }
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }

  if (sendQueue == nullptr)
  {
    std::cerr << "[INVENTORY] sendQueue nullptr. No se puede enviar acción."
              << std::endl;
    return;
  }

  // Por ahora, todo click sobre item equipable se manda al server.
  // El cliente NO equipa localmente.
  sendQueue->try_push(
      std::make_shared<const EquipItemMessage>(item.instanceId));
}

int Game::getEquipmentSlotIndexAt(int mouseX, int mouseY) const
{
  const int eqSlotSize = 58;
  const int eqGap = 12;
  const int eqY = 168;
  const int eqStartX = 956;

  for (int i = 0; i < 4; i++)
  {
    SDL_Rect slot = {eqStartX + i * (eqSlotSize + eqGap), eqY, eqSlotSize,
                     eqSlotSize};

    const bool inside = mouseX >= slot.x && mouseX < slot.x + slot.w &&
                        mouseY >= slot.y && mouseY < slot.y + slot.h;

    if (inside)
    {
      return i;
    }
  }

  return -1;
}

bool Game::addItemToFirstFreeInventorySlot(const ItemView &item)
{
  for (auto &slot : inventoryState.slots)
  {
    if (!slot.has_value())
    {
      slot = item;
      return true;
    }
  }

  return false;
}

void Game::handleEquipmentSlotClick(int equipmentSlotIndex)
{
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }

  if (sendQueue == nullptr)
  {
    std::cerr << "[EQUIPMENT] sendQueue nullptr. No se puede desequipar."
              << std::endl;
    return;
  }

  const auto maybeSlot = toClientEquipmentSlot(equipmentSlotIndex);

  if (!maybeSlot.has_value())
  {
    return;
  }

  const ClientEquipmentSlot visualSlot = maybeSlot.value();

  const std::optional<ItemView> *selectedSlot = nullptr;

  switch (visualSlot)
  {
  case ClientEquipmentSlot::Weapon:
    selectedSlot = &equipmentState.weapon;
    break;

  case ClientEquipmentSlot::Helmet:
    selectedSlot = &equipmentState.helmet;
    break;

  case ClientEquipmentSlot::Armor:
    selectedSlot = &equipmentState.armor;
    break;

  case ClientEquipmentSlot::Shield:
    selectedSlot = &equipmentState.shield;
    break;
  }

  if (selectedSlot == nullptr || !selectedSlot->has_value())
  {
    std::cout << "[EQUIPMENT] slot vacío visual=" << equipmentSlotIndex
              << std::endl;
    return;
  }

  const EquipSlot serverSlot = toServerEquipSlot(visualSlot);

  sendQueue->try_push(std::make_shared<const UnequipSlotMessage>(serverSlot));
}

void Game::consumePotion(int slotIndex)
{
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }
  // Validamos que el índice sea válido.
  if (slotIndex < 0 ||
      slotIndex >= static_cast<int>(inventoryState.slots.size()))
  {
    return;
  }

  // Si el slot está vacío, no hay nada para consumir.
  if (!inventoryState.slots[slotIndex].has_value())
  {
    return;
  }

  // Tomamos una copia modificable del ítem.
  ItemView item = inventoryState.slots[slotIndex].value();

  if (item.type == ClientItemType::HealthPotion)
  {
    // Calculamos nueva vida sin superar el máximo.
    playerState.hp += item.healAmount;

    if (playerState.hp > playerState.maxHp)
    {
      playerState.hp = playerState.maxHp;
    }


  }
  else if (item.type == ClientItemType::ManaPotion)
  {
    // Calculamos nuevo maná sin superar el máximo.
    playerState.mana += item.manaAmount;

    if (playerState.mana > playerState.maxMana)
    {
      playerState.mana = playerState.maxMana;
    }

  }
  else
  {
    // Si no era poción, no hacemos nada.
    return;
  }

  // Reducimos la cantidad.
  item.quantity--;

  // Si se terminó, vaciamos el slot.
  if (item.quantity <= 0)
  {
    inventoryState.slots[slotIndex] = std::nullopt;
  }
  else
  {
    inventoryState.slots[slotIndex] = item;
  }
}

std::string Game::visualTextureForCurrentRace(const ItemView &item) const
{
  if (playerState.race == "Dwarf" || playerState.race == "Gnome")
  {
    if (!item.visualTextureIdShort.empty())
    {
      return item.visualTextureIdShort;
    }
  }

  if (!item.visualTextureIdTall.empty())
  {
    return item.visualTextureIdTall;
  }

  return item.visualTextureId;
}

void Game::renderEquippedArmor()
{
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }
  // Si no hay armadura equipada, no dibujamos nada.
  if (!equipmentState.armor.has_value())
  {
    return;
  }

  // Tomamos la armadura equipada.
  const ItemView &armor = equipmentState.armor.value();

  // Elegimos la textura visual correcta según la raza:
  // human/elf -> tall
  // dwarf/gnome -> short
  const std::string visualTextureId = visualTextureForCurrentRace(armor);

  SDL_Texture *armorTexture = assets->GetTexture(visualTextureId);

  if (armorTexture == nullptr)
  {
    std::cout << "[EQUIPMENT RENDER] No existe textura: " << visualTextureId
              << std::endl;
    return;
  }

  // Obtenemos el SpriteComponent del player para copiar su frame y posición.
  auto &sprite = player->getComponent<SpriteComponent>();

  const SDL_Rect &playerSrc = sprite.getSrcRect();
  const SDL_Rect &playerDest = sprite.getDestRect();

  // La armadura debe usar el mismo frame/dirección del cuerpo.
  SDL_Rect armorSrc = {playerSrc.x - sprite.getStartX(),
                       playerSrc.y - sprite.getStartY(), playerSrc.w,
                       playerSrc.h};

  // Copiamos la posición actual del jugador en pantalla.
  SDL_Rect armorDest = playerDest;

  SDL_Point armorOffset = visualOffsetForCurrentRace(armor);
  armorDest.x += armorOffset.x * armorSpriteConfigForCurrentRace().scale;
  armorDest.y += armorOffset.y * armorSpriteConfigForCurrentRace().scale;
  SDL_RenderCopy(renderer, armorTexture, &armorSrc, &armorDest);
}

SpriteSheetConfig Game::armorSpriteConfigForCurrentRace() const
{
  // Si no hay armadura equipada, devolvemos una config neutra.
  // En la práctica casi no debería entrar acá, porque este método
  // se llama cuando ya hay armadura.
  if (!equipmentState.armor.has_value())
  {
    return SpriteSheetConfig{
        27, // ancho de cada frame
        47, // alto de cada frame
        2,  // escala visual
        0,  // startX dentro del spritesheet de armadura
        0,  // startY dentro del spritesheet de armadura
        0,  // offset X
        0   // offset Y
    };
  }

  // Tomamos la armadura actualmente equipada.
  const ItemView &armor = equipmentState.armor.value();

  // Las razas bajas necesitan usar los offsets short.
  const bool isShortRace =
      playerState.race == "Dwarf" || playerState.race == "Gnome";

  // Devolvemos la config de la armadura, incluyendo offsets visuales.
  return SpriteSheetConfig{
      27, // ancho de cada frame
      47, // alto de cada frame
      2,  // escala visual
      0,  // startX: el spritesheet de armadura arranca en 0
      0,  // startY: el spritesheet de armadura arranca en 0

      // Si es dwarf/gnome, usa shortOffsetX.
      // Si no, usa tallOffsetX.
      isShortRace ? armor.visualShortOffsetX : armor.visualTallOffsetX,

      // Si es dwarf/gnome, usa shortOffsetY.
      // Si no, usa tallOffsetY.
      isShortRace ? armor.visualShortOffsetY : armor.visualTallOffsetY};
}

void Game::refreshPlayerBodySprite()
{
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }
  // Obtenemos el SpriteComponent del jugador local.
  auto &sprite = player->getComponent<SpriteComponent>();

  // Si hay armadura equipada, reemplazamos el cuerpo desnudo
  // por la textura visual de la armadura.
  if (equipmentState.armor.has_value())
  {
    const ItemView &armor = equipmentState.armor.value();

    // Elige armor_iron_tall para human/elf
    // y armor_iron_short para dwarf/gnome.
    const std::string armorTextureId = visualTextureForCurrentRace(armor);

    sprite.setSpriteTextureAndConfig(armorTextureId,
                                     armorSpriteConfigForCurrentRace());

    return;
  }

  sprite.setSpriteTextureAndConfig("body_sheet",
                                   assets->bodyConfigForRace(playerState.race));
}

// helpér
SDL_Point Game::visualOffsetForCurrentRace(const ItemView &item) const
{
  if (playerState.race == "Dwarf" || playerState.race == "Gnome")
  {
    return SDL_Point{item.visualShortOffsetX, item.visualShortOffsetY};
  }

  return SDL_Point{item.visualTallOffsetX, item.visualTallOffsetY};
}

void Game::refreshPlayerEquipmentVisuals()
{
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }
  // Obtenemos el SpriteComponent del jugador local.
  auto &sprite = player->getComponent<SpriteComponent>();

  // Casco / capucha.
  // Si hay casco equipado, usamos su textura visual y sus offsets.
  if (equipmentState.helmet.has_value())
  {
    const ItemView &helmet = equipmentState.helmet.value();

    sprite.setHelmetTexture(
        helmet.visualTextureId, helmet.visualOffsetX, helmet.visualOffsetY,
        helmet.iconSrcW, helmet.iconSrcH, helmet.visualDownSrcX,
        helmet.visualDownSrcY, helmet.visualLeftSrcX, helmet.visualLeftSrcY,
        helmet.visualRightSrcX, helmet.visualRightSrcY, helmet.visualUpSrcX,
        helmet.visualUpSrcY);
  }
  else
  {
    // Si no hay casco equipado, limpiamos el visual.
    sprite.clearHelmet();
  }
  // Arma y escudo: no necesitan limpiar nada en el sprite porque se
  // renderizan en render() chequeando equipmentState directamente.
  // Con que el slot esté vacío alcanza para que no se dibujen.
}

void Game::renderEquippedWeapon()
{
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }

  if (!equipmentState.weapon.has_value())
  {
    return;
  }

  const ItemView &weapon = equipmentState.weapon.value();

  const std::string &textureId = weapon.visualTextureId;
  if (textureId.empty())
  {
    return;
  }

  SDL_Texture *weaponTexture = assets->GetTexture(textureId);
  if (weaponTexture == nullptr)
  {
    std::cout << "[EQUIPMENT RENDER] No existe textura de arma: " << textureId
              << std::endl;
    return;
  }

  auto &sprite = player->getComponent<SpriteComponent>();
  const SDL_Rect &playerSrc = sprite.getSrcRect();
  const SDL_Rect &playerDest = sprite.getDestRect();

  SDL_Rect weaponSrc = {playerSrc.x - sprite.getStartX(),
                        playerSrc.y - sprite.getStartY(), playerSrc.w,
                        playerSrc.h};

  // El destRect debe tener el tamaño del frame escalado — no el del personaje.
  // playerDest.w/h heredan el tamaño del body (54x94 con scale 2), que es
  // el mismo que queremos para la espada.
  const SpriteSheetConfig cfg = armorSpriteConfigForCurrentRace();
  SDL_Point offset = visualOffsetForCurrentRace(weapon);

  SDL_Rect weaponDest = {playerDest.x + offset.x, playerDest.y + offset.y,
                         playerSrc.w * cfg.scale, playerSrc.h * cfg.scale};

  // SDL_RenderCopy(renderer, weaponTexture, &weaponSrc, &weaponDest);
  //  Usamos el mismo flip que el cuerpo del personaje para que
  //  el arma acompañe la orientación y quede siempre en la mano derecha.
  SDL_RenderCopyEx(renderer, weaponTexture, &weaponSrc, &weaponDest, 0, nullptr,
                   sprite.spriteFlip);
}

void Game::renderEquippedShield()
{
  if (isLocalPlayerDead())
  {
    showStatusMessage("No puedes usar objetos estando muerto");
    return;
  }
  if (!equipmentState.shield.has_value())
  {
    return;
  }

  const ItemView &shield = equipmentState.shield.value();

  const std::string &textureId = shield.visualTextureId;
  if (textureId.empty())
  {
    return;
  }

  SDL_Texture *shieldTexture = assets->GetTexture(textureId);
  if (shieldTexture == nullptr)
  {
    std::cout << "[EQUIPMENT RENDER] No existe textura de escudo: " << textureId
              << std::endl;
    return;
  }

  auto &sprite = player->getComponent<SpriteComponent>();
  const SDL_Rect &playerSrc = sprite.getSrcRect();
  const SDL_Rect &playerDest = sprite.getDestRect();

  SDL_Rect shieldSrc = {playerSrc.x - sprite.getStartX(),
                        playerSrc.y - sprite.getStartY(), playerSrc.w,
                        playerSrc.h};

  const SpriteSheetConfig cfg = armorSpriteConfigForCurrentRace();
  SDL_Point offset = visualOffsetForCurrentRace(shield);

  SDL_Rect shieldDest = {playerDest.x + offset.x, playerDest.y + offset.y,
                         playerSrc.w * cfg.scale, playerSrc.h * cfg.scale};

  // SDL_RenderCopy(renderer, shieldTexture, &shieldSrc, &shieldDest);
  SDL_RenderCopyEx(renderer, shieldTexture, &shieldSrc, &shieldDest, 0, nullptr,
                   sprite.spriteFlip);
}

void Game::renderEnemyHealthBars()
{
  for (const auto &[enemyId, enemy] : enemies)
  {
    if (enemy == nullptr)
    {
      continue;
    }

    if (attackSystem.isEnemyDead(enemyId))
    {
      continue;
    }

    auto &transform = enemy->getComponent<TransformComponent>();

    int currentHp = attackSystem.getEnemyHealth(enemyId);
    int maxHp = attackSystem.getEnemyMaxHealth(enemyId);

    if (maxHp <= 0)
    {
      continue;
    }

    // PERF: culling — no dibujar barras de NPCs fuera de pantalla.
    {
      int screenXCheck = static_cast<int>(transform.position.x) - camera.x;
      int screenYCheck =
          static_cast<int>(transform.position.y) - camera.y + 133;
      if (screenXCheck < -64 || screenXCheck > 964 || screenYCheck < -16 ||
          screenYCheck > 736)
      {
        continue;
      }
    }

    float hpRatio = static_cast<float>(currentHp) / static_cast<float>(maxHp);

    if (hpRatio < 0.0f)
    {
      hpRatio = 0.0f;
    }

    if (hpRatio > 1.0f)
    {
      hpRatio = 1.0f;
    }

    int screenX = static_cast<int>(transform.position.x) - camera.x;
    int screenY = static_cast<int>(transform.position.y) - camera.y + 133;

    const int barWidth = 50;
    const int barHeight = 6;

    // Ajuste vertical de la barra.
    // Más negativo = más arriba. Más positivo = más abajo.
    const int barOffsetY = -4;

    SDL_Rect backgroundBar{screenX, screenY + barOffsetY, barWidth, barHeight};

    SDL_Rect healthBar{screenX, screenY + barOffsetY,
                       static_cast<int>(barWidth * hpRatio), barHeight};

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &backgroundBar);

    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &healthBar);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &backgroundBar);
  }
}
bool Game::isLocalPlayerDead() const
{
  // Si el servidor ya marcó al jugador como fantasma, está muerto.
  if (playerState.isDead)
  {
    return true;
  }

  // Si todavía no recibimos una vida válida, no podemos asumir muerte
  // solo porque hp sea 0.
  if (!hasReceivedValidPlayerStats)
  {
    return false;
  }

  // Luego de recibir stats válidas, hp <= 0 sí representa muerte.
  return playerState.hp <= 0;
}

void Game::applyLocalPlayerGhostState()
{
  // Evita repetir esta lógica todos los frames.
  if (localGhostStateApplied)
  {
    return;
  }

  localGhostStateApplied = true;
  playerState.isDead = true;
  playerState.hp = 0;
  playerState.mana = 0;

  if (player != nullptr && player->hasComponent<EquipmentComponent>())
  {
    auto &equipment = player->getComponent<EquipmentComponent>();

    equipment.setWeapon(std::nullopt);
    equipment.setShield(std::nullopt);
    equipment.setArmor(std::nullopt);
    equipment.setHelmet(std::nullopt);
  }

  assets->applyGhostAppearance(*player);
  // Mensaje temporal para confirmar el estado.
  showStatusMessage("Has muerto");

}

void Game::reviveLocalPlayer(int newHp)
{
  // El jugador vuelve a estar vivo.
  playerState.isDead = false;

  // Permitimos que, si muere otra vez, se pueda aplicar de nuevo
  // la transición a fantasma.
  localGhostStateApplied = false;
  playerState.hp = newHp;

  // Mensaje visual temporal.
  showStatusMessage("Has revivido");

  std::cout << "[PLAYER] Revivió. HP=" << playerState.hp << std::endl;
  assets->applyPlayerAppearance(*player, playerState);
  refreshPlayerEquipmentVisuals();
}

bool Game::sameColor(SDL_Color a, SDL_Color b) const
{
  // Compara color completo, incluido alpha.
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

SDL_Texture *Game::getOrCreateTextTexture(const std::string &key,
                                          const std::string &text,
                                          TTF_Font *font, SDL_Color color,
                                          int &outW, int &outH)
{
  // Buscamos si ya existe una textura cacheada para esta key lógica.
  auto it = textCache.find(key);

  if (it != textCache.end())
  {
    CachedText &cached = it->second;

    // Si texto, fuente y color siguen iguales, reutilizamos la textura.
    if (cached.texture != nullptr && cached.text == text &&
        cached.font == font && sameColor(cached.color, color))
    {
      outW = cached.w;
      outH = cached.h;
      return cached.texture;
    }

    // Si cambió algo, destruimos la textura anterior.
    if (cached.texture != nullptr)
    {
      SDL_DestroyTexture(cached.texture);
      cached.texture = nullptr;
    }
  }

  // Si el texto está vacío, no generamos textura.
  if (text.empty() || font == nullptr)
  {
    outW = 0;
    outH = 0;
    return nullptr;
  }

  // Creamos surface nueva solo cuando el texto realmente cambió.
  SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), color);
  if (surface == nullptr)
  {
    outW = 0;
    outH = 0;
    return nullptr;
  }

  // Convertimos surface a texture.
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (texture == nullptr)
  {
    SDL_FreeSurface(surface);
    outW = 0;
    outH = 0;
    return nullptr;
  }

  CachedText cached;
  cached.text = text;
  cached.color = color;
  cached.font = font;
  cached.texture = texture;
  cached.w = surface->w;
  cached.h = surface->h;

  SDL_FreeSurface(surface);

  // Guardamos o reemplazamos la entrada cacheada.
  textCache[key] = cached;

  outW = cached.w;
  outH = cached.h;

  return texture;
}

void Game::clearTextCache()
{
  // Destruimos todas las texturas cacheadas.
  for (auto &[key, cached] : textCache)
  {
    if (cached.texture != nullptr)
    {
      SDL_DestroyTexture(cached.texture);
      cached.texture = nullptr;
    }
  }

  textCache.clear();
}

void Game::applyInventoryUpdate(const InventoryUpdateMessage &msg)
{
  // Limpiamos el inventario visual actual.
  // El server manda el estado completo, así que reconstruimos todo desde cero.
  for (auto &slot : inventoryState.slots)
  {
    slot.reset();
  }

  // Limpiamos el equipamiento visual actual.
  // Luego lo reconstruimos usando msg.getEquipped().
  equipmentState.weapon.reset();
  equipmentState.helmet.reset();
  equipmentState.armor.reset();
  equipmentState.shield.reset();

  // Lista completa de items reales que tiene el jugador en el server.
  const auto &serverItems = msg.getItems();

  // Slots reales del inventario, decididos por el server.
  // Cada posición guarda un instanceId o 0 si está vacía.
  const auto &inventorySlots = msg.getInventorySlots();

  // Array de equipados.
  // Cada posición guarda un instanceId equipado o 0 si el slot está vacío.
  const auto &equipped = msg.getEquipped();

  // Creamos un índice rápido:
  // instanceId -> Item*
  // Así podemos buscar cada item por id sin recorrer el vector muchas veces.
  std::unordered_map<uint32_t, const Item *> itemByInstanceId;

  for (const Item &item : serverItems)
  {
    itemByInstanceId[item.instanceId] = &item;
  }

  // Función auxiliar:
  // Convierte un Item del server en un ItemView del cliente.
  auto makeItemView = [&](const Item &serverItem) -> std::optional<ItemView>
  {
    try
    {
      // catalogId permite buscar la metadata visual en items.json.
      ItemView view =
          itemCatalog.requireById(static_cast<int>(serverItem.catalogId));

      // instanceId identifica esta instancia real del server.
      view.instanceId = serverItem.instanceId;

      return view;
    }
    catch (const std::exception &e)
    {
      std::cerr << "[CLIENT][INV] catalogId desconocido="
                << serverItem.catalogId
                << " instanceId=" << serverItem.instanceId
                << " typeName=" << serverItem.typeName << " error=" << e.what()
                << std::endl;

      return std::nullopt;
    }
  };

  // 1. Aplicamos los slots de inventario EXACTAMENTE como los manda el server.
  for (std::size_t slotIndex = 0; slotIndex < inventorySlots.size() &&
                                  slotIndex < inventoryState.slots.size();
       ++slotIndex)
  {

    const uint32_t itemInstanceId = inventorySlots[slotIndex];

    // 0 significa slot vacío.
    if (itemInstanceId == 0)
    {
      inventoryState.slots[slotIndex].reset();
      continue;
    }

    // Buscamos ese instanceId en los items enviados por el server.
    auto it = itemByInstanceId.find(itemInstanceId);

    if (it == itemByInstanceId.end())
    {
      inventoryState.slots[slotIndex].reset();

      std::cerr << "[CLIENT][INV] slot=" << slotIndex
                << " apunta a instanceId inexistente=" << itemInstanceId
                << std::endl;
      continue;
    }

    // Convertimos Item server -> ItemView cliente.
    std::optional<ItemView> view = makeItemView(*it->second);

    if (!view.has_value())
    {
      inventoryState.slots[slotIndex].reset();
      continue;
    }

    // Dibujamos el item en el slot exacto que mandó el server.
    inventoryState.slots[slotIndex] = view.value();
  }

  // 2. Aplicamos equipamiento.
  auto applyEquipped = [&](EquipSlot slot, std::optional<ItemView> &target)
  {
    const auto index = static_cast<std::size_t>(slot);

    if (index >= equipped.size())
    {
      target.reset();
      return;
    }

    const uint32_t equippedInstanceId = equipped[index];

    // 0 significa slot de equipo vacío.
    if (equippedInstanceId == 0)
    {
      target.reset();
      return;
    }

    // Buscamos el item equipado en la lista completa del server.
    auto it = itemByInstanceId.find(equippedInstanceId);

    if (it == itemByInstanceId.end())
    {
      target.reset();

      std::cerr << "[CLIENT][EQUIP] instanceId equipado no vino en items. id="
                << equippedInstanceId << std::endl;
      return;
    }

    std::optional<ItemView> view = makeItemView(*it->second);

    if (!view.has_value())
    {
      target.reset();
      return;
    }

    target = view.value();
  };

  applyEquipped(EquipSlot::HAND, equipmentState.weapon);
  applyEquipped(EquipSlot::HELMET, equipmentState.helmet);
  applyEquipped(EquipSlot::ARMOR, equipmentState.armor);
  applyEquipped(EquipSlot::SHIELD, equipmentState.shield);

  if (player != nullptr && player->hasComponent<EquipmentComponent>() &&
      !playerState.isDead)
  {
    auto &equipment = player->getComponent<EquipmentComponent>();

    equipment.setWeapon(equipmentState.weapon);
    equipment.setArmor(equipmentState.armor);
    equipment.setHelmet(equipmentState.helmet);
    equipment.setShield(equipmentState.shield);
  }
  // Refrescamos visuales.
  // refreshPlayerEquipmentVisuals();
  // refreshPlayerBodySprite();
}

void Game::handleEntityMove(const EntityMoveMessage &moveMsg)
{
  const uint32_t entityId = static_cast<uint32_t>(moveMsg.getId());
  const float serverX = static_cast<float>(moveMsg.getX());
  const float serverY = static_cast<float>(moveMsg.getY());
  const Direction direction = moveMsg.getDirection();
  const bool moving = moveMsg.isMoving();

  if (clientWorld != nullptr)
  {
    clientWorld->updatePlayerPosition(entityId, serverX, serverY, direction,
                                      moving);
  }
}
void Game::handlePlayerDied(const PlayerDiedMessage &diedMsg)
{
  // ID del jugador muerto enviado por el server.
  const uint32_t deadPlayerId = diedMsg.getId();


  // Si el muerto soy yo, aplico estado fantasma local.
  if (deadPlayerId == static_cast<uint32_t>(playerDto.playerID))
  {
    playerState.isDead = true;
    playerState.hp = 0;
    playerState.mana = 0;
    applyLocalPlayerGhostState();
    return;
  }

  // Si murió otro jugador, hay que actualizar su entidad remota.
  if (clientWorld != nullptr)
  {
    clientWorld->applyRemotePlayerGhostState(deadPlayerId);
  }
}
void Game::handlePlayerStats(const PlayerStatsMessage &stats)
{
  const int serverHp = stats.getHp();

  if (serverHp > 0)
  {
    hasReceivedValidPlayerStats = true;
  }

  playerState.maxHp = stats.getMaxHp();
  playerState.mana = stats.getMana();
  playerState.maxMana = stats.getMaxMana();
  playerState.exp = stats.getExp();
  playerState.expToNextLevel = stats.getExpLimit();
  playerState.level = stats.getLevel();
  playerState.gold = stats.getGold();

  // Si el server dice HP 0, el jugador local debe quedar fantasma,
  // aunque todavía no haya llegado o ya haya llegado MSG_PLAYER_DIED.
  if (serverHp <= 0)
  {
    playerState.isDead = true;
    playerState.hp = 0;
    playerState.mana = 0;
    applyLocalPlayerGhostState();
    return;
  }

  // Si estaba muerto y ahora viene HP positivo, es resurrección.
  if (playerState.isDead && serverHp > 0)
  {
    reviveLocalPlayer(serverHp);
    return;
  }

  playerState.hp = serverHp;
}
void Game::handleEntitySpawn(const EntitySpawnMessage &spawnMsg)
{

  const PlayerDto &dto = spawnMsg.getPlayerDto();

  std::cout << "[CLIENT] MSG_ENTITY_SPAWN recibido. playerID="
            << static_cast<int>(dto.playerID)
            << " localID=" << static_cast<int>(playerDto.playerID) << " pos=("
            << dto.xpos << ", " << dto.ypos << ")" << std::endl;

  if (clientWorld != nullptr)
  {
    clientWorld->spawnRemotePlayer(dto);
  }
  std::cout << "[CLIENT] MSG_ENTITY_SPAWN recibido. playerID="
            << static_cast<int>(dto.playerID)
            << " localID=" << static_cast<int>(playerDto.playerID) << " pos=("
            << dto.xpos << ", " << dto.ypos << ")"
            << " esFantasma=" << dto.esFantasma << std::endl;
}
void Game::handleInventoryUpdate(const InventoryUpdateMessage &inventoryMsg)
{

  applyInventoryUpdate(inventoryMsg);

  std::cout << "[CLIENT] MSG_INVENTORY_UPDATE recibido. items="
            << inventoryMsg.getItems().size() << std::endl;
  if (playerState.isDead || playerState.hp <= 0)
  {
    applyLocalPlayerGhostState();
  }
}

void Game::processServerMessage(const Message &msg)
{
  switch (static_cast<ServerOpCode>(msg.opCode()))
  {
  case ServerOpCode::MSG_ENTITY_MOVE:
    handleEntityMove(static_cast<const EntityMoveMessage &>(msg));
    return;

  case ServerOpCode::MSG_PLAYER_DIED:
    handlePlayerDied(static_cast<const PlayerDiedMessage &>(msg));
    return;

  case ServerOpCode::MSG_PLAYER_STATS:
    handlePlayerStats(static_cast<const PlayerStatsMessage &>(msg));
    return;

  case ServerOpCode::MSG_ENTITY_SPAWN:
    handleEntitySpawn(static_cast<const EntitySpawnMessage &>(msg));
    return;

  case ServerOpCode::MSG_ENTITY_DESPAWN:
    handleEntityDespawn(static_cast<const EntityDespawnMessage &>(msg));
    return;

  case ServerOpCode::MSG_INVENTORY_UPDATE:
    handleInventoryUpdate(static_cast<const InventoryUpdateMessage &>(msg));
    return;
  case ServerOpCode::MSG_PLAYER_EQUIPMENT_UPDATE:
    handlePlayerEquipmentUpdate(
        static_cast<const PlayerEquipmentUpdateMessage &>(msg));
    return;
  case ServerOpCode::MSG_LEVEL_UP:
    handleLevelUp(static_cast<const LevelUpMessage &>(msg));
    return;
  case ServerOpCode::MSG_NPC_SPAWN:
    handleNpcSpawn(static_cast<const NpcSpawnMessage &>(msg));
    return;
  case ServerOpCode::MSG_NPC_HEALTH:
    handleNpcHealth(static_cast<const NpcHealthMessage &>(msg));
    return;
  case ServerOpCode::MSG_NPC_MOVE:
    handleNpcMove(static_cast<const NpcMoveMessage &>(msg));
    return;
  case ServerOpCode::MSG_PLAYER_RESURRECTED:
    handlePlayerResurrected(static_cast<const PlayerResurrectedMessage &>(msg));
    return;
  case ServerOpCode::MSG_MAP_CHANGED:
    handleMapChanged(static_cast<const MapChangedMessage &>(msg));
    return;
  case ServerOpCode::MSG_ITEM_ON_GROUND:
    handleItemOnGround(static_cast<const ItemOnGroundMessage &>(msg));
    return;

  case ServerOpCode::MSG_GOLD_ON_GROUND:
    handleGoldOnGround(static_cast<const GoldOnGroundMessage &>(msg));
    return;

  case ServerOpCode::MSG_ITEM_PICKED:
    handleItemPicked(static_cast<const ItemPickedMessage &>(msg));
    return;

    default:
      return;
  }
}
std::optional<ClientEquipmentSlot>
Game::toClientEquipmentSlot(int index) const
{
  switch (index)
  {
  case static_cast<int>(ClientEquipmentSlot::Weapon):
    return ClientEquipmentSlot::Weapon;

  case static_cast<int>(ClientEquipmentSlot::Helmet):
    return ClientEquipmentSlot::Helmet;

  case static_cast<int>(ClientEquipmentSlot::Armor):
    return ClientEquipmentSlot::Armor;

  case static_cast<int>(ClientEquipmentSlot::Shield):
    return ClientEquipmentSlot::Shield;

  default:
    return std::nullopt;
  }
}

EquipSlot Game::toServerEquipSlot(ClientEquipmentSlot slot) const
{
  switch (slot)
  {
  case ClientEquipmentSlot::Weapon:
    return EquipSlot::HAND;

  case ClientEquipmentSlot::Helmet:
    return EquipSlot::HELMET;

  case ClientEquipmentSlot::Armor:
    return EquipSlot::ARMOR;

  case ClientEquipmentSlot::Shield:
    return EquipSlot::SHIELD;
  }

  return EquipSlot::HAND;
}

void Game::handlePlayerEquipmentUpdate(
    const PlayerEquipmentUpdateMessage &msg)
{
  const uint32_t updatedPlayerId = msg.getPlayerId();

  // El jugador local ya se actualiza mediante InventoryUpdateMessage.
  // Este mensaje se usa para actualizar jugadores remotos.
  if (updatedPlayerId == static_cast<uint32_t>(playerDto.playerID))
  {
    return;
  }

  if (clientWorld == nullptr)
  {
    return;
  }

  clientWorld->updateRemotePlayerEquipment(updatedPlayerId, msg.getEquipment(),
                                           itemCatalog);
}

void Game::handleLevelUp(const LevelUpMessage &msg)
{
  const uint32_t updatedPlayerId = msg.getPlayerId();
  const uint32_t newLevel = msg.getLevel();

  if (updatedPlayerId == static_cast<uint32_t>(playerDto.playerID))
  {
    playerState.level = newLevel;
    return;
  }

  if (clientWorld != nullptr)
  {
    clientWorld->updateRemotePlayerLevel(updatedPlayerId, newLevel);
  }
}

void Game::handleNpcSpawn(const NpcSpawnMessage &msg)
{
  auto existing = enemies.find(msg.getNpcId());

  if (existing != enemies.end())
  {
    Entity *enemyEntity = existing->second;

    if (enemyEntity != nullptr)
    {
      // Respawn de NPC existente:
      // actualizamos posición y vida, sin crear una entidad duplicada.
      auto &transform = enemyEntity->getComponent<TransformComponent>();

      transform.position.x = static_cast<float>(msg.getX());
      transform.position.y = static_cast<float>(msg.getY());

      attackSystem.setEnemyHealth(msg.getNpcId(), static_cast<int>(msg.getHp()),
                                  static_cast<int>(msg.getHpMax()));


      return;
    }

    enemies.erase(existing);
  }

  NPCData npcData{};

  // Adaptamos desde el mensaje de red al DTO visual del cliente.
  npcData.npcID = msg.getNpcId();
  npcData.type = msg.getType();
  npcData.nombre = msg.getName();

  npcData.x = msg.getX();
  npcData.y = msg.getY();

  npcData.hp = static_cast<int>(msg.getHp());
  npcData.hpMax = static_cast<int>(msg.getHpMax());

  npcData.estaVivo = msg.getHp() > 0;
  npcData.estaMoviendo = false;
  npcData.hostile = msg.isHostile();

  Entity *npcEntity = nullptr;

  if (npcData.hostile)
  {
    // Enemigo de combate: va al grupo enemies y tiene barra de vida.
    npcEntity = assets->CreateEnemy(npcData);

    if (npcEntity == nullptr)
    {
      std::cout << "[CLIENT NPC] no se pudo crear npcId=" << npcData.npcID
                << " nombre=" << npcData.nombre << std::endl;
      return;
    }

    enemies[msg.getNpcId()] = npcEntity;

    attackSystem.setEnemyHealth(msg.getNpcId(), npcData.hp, npcData.hpMax);
  }
  else
  {
    // NPC de ciudad (priest, merchant, banker): va al grupo NPC, sin barra de
    // vida.
    npcEntity = assets->CreateNpc(npcData);

    if (npcEntity == nullptr)
    {

      return;
    }
  }
}

void Game::handleNpcHealth(const NpcHealthMessage &msg)
{
  const uint32_t npcId = msg.getNpcId();

  attackSystem.setEnemyHealth(npcId, msg.getHp(), msg.getMaxHp());

  if (msg.getHp() <= 0)
  {
    auto it = enemies.find(npcId);

    if (it != enemies.end())
    {
      enemies.erase(it);
    }
  }
}

void Game::handleNpcMove(const NpcMoveMessage &msg)
{
  const uint32_t npcId = msg.getNpcId();

  // Buscamos el enemigo en el mapa visual.
  auto it = enemies.find(npcId);

  // Si no existe en cliente, no podemos moverlo.
  // En ese caso debería llegar primero un NpcSpawnMessage.
  if (it == enemies.end())
  {

    return;
  }

  Entity *enemyEntity = it->second;

  if (enemyEntity == nullptr)
  {

    return;
  }

  auto &transform = enemyEntity->getComponent<TransformComponent>();

  const float newX = static_cast<float>(msg.getX());
  const float newY = static_cast<float>(msg.getY());

  // Actualizamos posición visual.
  transform.position.x = newX;
  transform.position.y = newY;
}

void Game::handlePlayerResurrected(const PlayerResurrectedMessage &msg)
{
  const uint32_t resurrectedId = msg.getPlayerId();

  // Si el revivido soy yo, restauro mi jugador local.
  if (resurrectedId == static_cast<uint32_t>(playerDto.playerID))
  {
    localGhostStateApplied = false;

    // Si tu reviveLocalPlayer espera HP, usá el HP actual del playerState
    // o un valor mínimo. Después PlayerStatsMessage va a corregirlo.
    reviveLocalPlayer(playerState.hp > 0 ? playerState.hp : 1);
    return;
  }

  // Si revivió otro jugador, restauro su sprite remoto.
  if (clientWorld != nullptr)
  {
    clientWorld->applyRemotePlayerAliveState(resurrectedId);
  }
}

void Game::drawEquippedEntity(Entity *entity, RenderContext &context)
{
  // Si la entidad no existe, no dibujamos nada.
  if (entity == nullptr)
  {
    return;
  }

  // Primera capa:
  // arma/escudo que deben quedar detrás del cuerpo.
  if (entity->hasComponent<EquipmentComponent>())
  {
    entity->getComponent<EquipmentComponent>().drawBehind(context);
  }

  // Segunda capa:
  // sprite principal del personaje: cuerpo, cabeza, casco.
  if (entity->hasComponent<SpriteComponent>())
  {
    entity->getComponent<SpriteComponent>().draw(context);
  }

  // Tercera capa:
  // arma/escudo que deben quedar delante del cuerpo.
  if (entity->hasComponent<EquipmentComponent>())
  {
    entity->getComponent<EquipmentComponent>().drawFront(context);
  }
}

void Game::handleEntityDespawn(const EntityDespawnMessage &msg)
{
  uint32_t idToRemove = msg.getEntityId();

  auto itEnemy = enemies.find(idToRemove);
  if (itEnemy != enemies.end())
  {
    if (itEnemy->second != nullptr)
    {
      itEnemy->second->destroy();
    }
    enemies.erase(itEnemy);
    std::cout << "[CLIENT] Enemigo despawneado ID: " << idToRemove << std::endl;
    return;
  }

  if (clientWorld != nullptr)
  {
    clientWorld->removeRemotePlayer(idToRemove);
  }
}

void Game::clearCurrentScene()
{
  for (auto &t : manager.getGroup(groupMap))
  {
    t->destroy();
  }
  for (auto &t : manager.getGroup(groupMapTop))
  {
    t->destroy();
  }

  for (auto &n : manager.getGroup(groupNPC))
  {
    n->destroy();
  }

  for (auto &ui : manager.getGroup(groupUI))
  {
    ui->destroy();
  }

  for (auto &[enemyId, enemyEntity] : enemies)
  {
    if (enemyEntity != nullptr)
    {
      enemyEntity->destroy();
    }
  }
  enemies.clear();

  for (auto &p : manager.getGroup(groupPlayers))
  {
    if (p != player)
    {
      p->destroy();
    }
  }

  for (auto &slot : inventoryState.slots)
  {
    slot = std::nullopt;
  }

  equipmentState.weapon = std::nullopt;
  equipmentState.helmet = std::nullopt;
  equipmentState.armor = std::nullopt;
  equipmentState.shield = std::nullopt;

  manager.refresh();

  if (clientWorld != nullptr)
  {
    clientWorld = std::make_unique<ClientGameWorld>(
        static_cast<uint32_t>(playerDto.playerID), player, *assets);
  }
}

void Game::handleMapChanged(const MapChangedMessage &msg)
{
  std::cout << "[Game] Cambiando al mapa: " << msg.getMapPath() << std::endl;

  clearCurrentScene();

  if (map != nullptr)
  {
    delete map;
    map = nullptr;
  }

  map = new Map(manager, *assets, "terrain", 3, 32);
  map->LoadMap(msg.getMapPath());

  std::cout << "[Game] ¡Nuevo mapa cargado exitosamente!" << std::endl;
}


void Game::handleItemOnGround(const ItemOnGroundMessage &msg) {
  const Item &item = msg.getItem();

  const ItemView *itemView = itemCatalog.getById(static_cast<int>(item.catalogId));

  if (itemView == nullptr) {
    std::cout << "[GROUND ITEM] catalogId desconocido: "
              << item.catalogId << std::endl;
    return;
  }

  const int pixelX = msg.getX() * 96;
  const int pixelY = msg.getY() * 96;

  Entity *entity = assets->CreateGroundItem(*itemView, pixelX, pixelY);

  groundItems[item.instanceId] = entity;

  std::cout << "[GROUND ITEM] creado instanceId=" << item.instanceId
            << " catalogId=" << item.catalogId
            << " tile=(" << msg.getX() << "," << msg.getY() << ")"
            << " pixel=(" << pixelX << "," << pixelY << ")"
            << std::endl;
}

void Game::handleGoldOnGround(const GoldOnGroundMessage &msg) {
  const ItemView *goldView = itemCatalog.getById(23);

  if (goldView == nullptr) {
    std::cout << "[GROUND GOLD] no se encontro catalogId=23 para oro" << std::endl;
    return;
  }

  const int pixelX = msg.getX() * 96;
  const int pixelY = msg.getY() * 96;

  Entity *entity = assets->CreateGroundItem(*goldView, pixelX, pixelY);

  groundGold[msg.getInstanceId()] = entity;

  std::cout << "[GROUND GOLD] creado instanceId=" << msg.getInstanceId()
            << " amount=" << msg.getAmount()
            << " tile=(" << msg.getX() << "," << msg.getY() << ")"
            << " pixel=(" << pixelX << "," << pixelY << ")"
            << std::endl;
}

void Game::handleItemPicked(const ItemPickedMessage &msg) {
  const uint32_t itemId = msg.getItemId();

  auto itemIt = groundItems.find(itemId);
  if (itemIt != groundItems.end()) {
    if (itemIt->second != nullptr) {
      itemIt->second->destroy();
    }
    groundItems.erase(itemIt);
    std::cout << "[GROUND ITEM] removido instanceId=" << itemId << std::endl;
    return;
  }

  auto goldIt = groundGold.find(itemId);
  if (goldIt != groundGold.end()) {
    if (goldIt->second != nullptr) {
      goldIt->second->destroy();
    }
    groundGold.erase(goldIt);
    std::cout << "[GROUND GOLD] removido instanceId=" << itemId << std::endl;
  }
}