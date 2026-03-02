// ============================================================
// Plan de Plantation Réseau — Main Application
// ============================================================

(function () {
  'use strict';

  // ── Constants ──
  const DEVICE_TYPES = {
    pc: { icon: '🖥️', label: 'Ordinateur' },
    laptop: { icon: '💻', label: 'Laptop' },
    regie: { icon: '🎛️', label: 'Régie' },
    server: { icon: '🖴', label: 'Serveur' },
    wifi: { icon: '📡', label: 'Antenne WiFi' },
    switch: { icon: '🔀', label: 'Switch' },
    router: { icon: '🌐', label: 'Routeur' },
    esp32: { icon: '🔌', label: 'ESP32' },
    projector: { icon: '📽️', label: 'Projecteur' },
    phone: { icon: '📱', label: 'Téléphone' },
    camera: { icon: '📷', label: 'Caméra' },
    soundcard: { icon: '🎵', label: 'Carte de Son' },
    // Backward compat for old JSON files
    printer: { icon: '🔌', label: 'ESP32' },
    screen: { icon: '📽️', label: 'Projecteur' },
    speaker: { icon: '🎵', label: 'Carte de Son' },
  };

  const DASH_PATTERNS = {
    solid: '',
    dashed: '12 6',
    dotted: '3 6',
    dashdot: '12 4 3 4',
  };

  const PATTERN_LABELS = {
    solid: 'Plein',
    dashed: 'Tirets',
    dotted: 'Pointillé',
    dashdot: 'Tiret-Point',
  };

  const DEVICE_SIZE = 60;

  // ── State ──
  let devices = [];
  let connections = [];
  let nextDeviceId = 1;
  let nextConnId = 1;

  let currentMode = 'select';
  let selectedDeviceType = null;
  let selectedDeviceId = null;
  let selectedDeviceIds = new Set(); // group selection
  let selectedConnectionId = null;
  let connectSourceId = null;
  let selectedPortNumber = null;

  // Rubber band selection
  let isRubberBanding = false;
  let rubberBandStart = null;
  let rubberBandRect = null;
  let isSpaceHeld = false; // Space = pan mode

  let isDragging = false;
  let dragDeviceId = null;
  let dragOffsetX = 0;
  let dragOffsetY = 0;

  let viewBox = { x: 0, y: 0, w: 0, h: 0 };
  let zoomLevel = 1;
  let isPanning = false;
  let panStartX = 0;
  let panStartY = 0;
  let panStartVBX = 0;
  let panStartVBY = 0;

  // ── DOM Refs ──
  const svg = document.getElementById('canvas-svg');
  const devicesLayer = document.getElementById('devices-layer');
  const connectionsLayer = document.getElementById('connections-layer');
  const tempConnLayer = document.getElementById('temp-connection-layer');
  const palette = document.getElementById('palette');
  const statusBar = document.getElementById('status-bar');
  const modeText = document.getElementById('mode-text');
  const toastContainer = document.getElementById('toast-container');

  const btnConnect = document.getElementById('btn-connect');
  const btnDelete = document.getElementById('btn-delete');
  const btnClear = document.getElementById('btn-clear');
  const btnSave = document.getElementById('btn-save');
  const btnLoad = document.getElementById('btn-load');
  const btnExport = document.getElementById('btn-export');
  const btnZoomIn = document.getElementById('btn-zoom-in');
  const btnZoomOut = document.getElementById('btn-zoom-out');
  const btnZoomReset = document.getElementById('btn-zoom-reset');
  const fileInput = document.getElementById('file-input');

  // Connection popover
  const popoverOverlay = document.getElementById('popover-overlay');
  const popover = document.getElementById('connection-popover');
  const popoverClose = document.getElementById('popover-close');
  const connNameInput = document.getElementById('conn-name');
  const connColorInput = document.getElementById('conn-color');
  const connPatternSelect = document.getElementById('conn-pattern');
  const connSaveBtn = document.getElementById('conn-save');
  const connDeleteBtn = document.getElementById('conn-delete');
  const colorPresets = document.getElementById('color-presets');

  // Right inspector panel
  const inspectorEmpty = document.getElementById('inspector-empty');
  const inspectorContent = document.getElementById('inspector-content');
  const inspIcon = document.getElementById('insp-icon');
  const inspType = document.getElementById('insp-type');
  const inspTypeSelect = document.getElementById('insp-type-select');
  const inspId = document.getElementById('insp-id');
  const inspName = document.getElementById('insp-name');
  const inspColor = document.getElementById('insp-color');
  const inspNotes = document.getElementById('insp-notes');
  const inspConnCount = document.getElementById('insp-conn-count');
  const inspConnections = document.getElementById('insp-connections');
  const inspColorPresets = document.getElementById('insp-color-presets');
  const inspApply = document.getElementById('insp-apply');
  const inspDeleteDevice = document.getElementById('insp-delete-device');

  // Port grid
  const inspPortsSection = document.getElementById('insp-ports-section');
  const inspPortCount = document.getElementById('insp-port-count');
  const inspPortGrid = document.getElementById('insp-port-grid');
  const portAssignPanel = document.getElementById('port-assign-panel');
  const portAssignNumber = document.getElementById('port-assign-number');
  const portAssignSelect = document.getElementById('port-assign-select');
  const portAssignCancel = document.getElementById('port-assign-cancel');
  const portAssignOk = document.getElementById('port-assign-ok');
  const inspPortNumbering = document.getElementById('insp-port-numbering');
  const inspNumberingField = document.getElementById('insp-numbering-field');
  const inspSfpField = document.getElementById('insp-sfp-field');
  const inspSfpToggle = document.getElementById('insp-sfp-toggle');
  const portAssignVlan = document.getElementById('port-assign-vlan');
  const portAssignColor = document.getElementById('port-assign-color');
  const portColorPresets = document.getElementById('port-color-presets');

  // ── Helpers ──
  function uuid() {
    return 'd' + (++nextDeviceId) + '_' + Date.now().toString(36);
  }

  function connUuid() {
    return 'c' + (++nextConnId) + '_' + Date.now().toString(36);
  }

  function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.textContent = message;
    toastContainer.appendChild(toast);
    setTimeout(() => {
      toast.style.animation = 'toastOut 0.3s ease-in forwards';
      setTimeout(() => toast.remove(), 300);
    }, 2500);
  }

  function showStatus(text) {
    statusBar.textContent = text;
    statusBar.classList.add('visible');
    clearTimeout(showStatus._timer);
    showStatus._timer = setTimeout(() => statusBar.classList.remove('visible'), 3000);
  }

  function setMode(mode) {
    currentMode = mode;
    svg.className.baseVal = `mode-${mode}`;

    palette.querySelectorAll('.palette-item').forEach(el => el.classList.remove('active'));
    btnConnect.classList.remove('active');

    if (mode === 'place' && selectedDeviceType) {
      const item = palette.querySelector(`[data-type="${selectedDeviceType}"]`);
      if (item) item.classList.add('active');
      modeText.textContent = `Placer: ${DEVICE_TYPES[selectedDeviceType].label}`;
    } else if (mode === 'connect') {
      btnConnect.classList.add('active');
      modeText.textContent = 'Cliquez un appareil source, puis la cible';
    } else {
      selectedDeviceType = null;
      modeText.textContent = 'Sélectionner un équipement';
    }
  }

  function getSVGPoint(clientX, clientY) {
    const pt = svg.createSVGPoint();
    pt.x = clientX;
    pt.y = clientY;
    return pt.matrixTransform(svg.getScreenCTM().inverse());
  }

  function getDeviceCenter(device) {
    return { x: device.x + DEVICE_SIZE / 2, y: device.y + DEVICE_SIZE / 2 };
  }

  function findDevice(id) { return devices.find(d => d.id === id); }
  function findConnection(id) { return connections.find(c => c.id === id); }

  function getDeviceConnections(deviceId) {
    return connections.filter(c => c.from === deviceId || c.to === deviceId);
  }

  // ── ViewBox ──
  function initViewBox() {
    const rect = svg.getBoundingClientRect();
    viewBox.w = rect.width;
    viewBox.h = rect.height;
    viewBox.x = 0;
    viewBox.y = 0;
    applyViewBox();
  }

  function applyViewBox() {
    svg.setAttribute('viewBox', `${viewBox.x} ${viewBox.y} ${viewBox.w} ${viewBox.h}`);
  }

  // ── Rendering ──
  function renderAll() {
    renderConnections();
    renderDevices();
    updateInspector();
  }

  function renderDevices() {
    devicesLayer.innerHTML = '';
    devices.forEach(device => {
      const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      const isSel = device.id === selectedDeviceId || selectedDeviceIds.has(device.id);
      g.setAttribute('class', `device-group ${isSel ? 'selected' : ''}`);
      g.setAttribute('data-id', device.id);
      g.setAttribute('transform', `translate(${device.x}, ${device.y})`);

      const bgColor = device.color || 'rgba(36, 40, 54, 0.9)';
      const bg = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      bg.setAttribute('class', 'device-bg');
      bg.setAttribute('width', DEVICE_SIZE);
      bg.setAttribute('height', DEVICE_SIZE);
      bg.setAttribute('rx', '12');
      bg.setAttribute('ry', '12');
      bg.setAttribute('fill', bgColor);
      bg.setAttribute('stroke', isSel ? '#4e8cff' : 'rgba(255,255,255,0.12)');
      bg.setAttribute('stroke-width', isSel ? '2' : '1');
      g.appendChild(bg);

      const icon = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      icon.setAttribute('class', 'device-icon');
      icon.setAttribute('x', DEVICE_SIZE / 2);
      icon.setAttribute('y', DEVICE_SIZE / 2 + 2);
      icon.setAttribute('text-anchor', 'middle');
      icon.setAttribute('dominant-baseline', 'central');
      icon.setAttribute('font-size', '26');
      icon.textContent = DEVICE_TYPES[device.type]?.icon || '❓';
      g.appendChild(icon);

      const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      label.setAttribute('class', 'device-label');
      label.setAttribute('x', DEVICE_SIZE / 2);
      label.setAttribute('y', DEVICE_SIZE + 16);
      label.setAttribute('text-anchor', 'middle');
      label.setAttribute('font-size', '11');
      label.setAttribute('fill', '#9aa0b0');
      label.textContent = device.label;
      g.appendChild(label);

      if (device.notes) {
        const noteIcon = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        noteIcon.setAttribute('class', 'device-notes-indicator');
        noteIcon.setAttribute('x', DEVICE_SIZE - 4);
        noteIcon.setAttribute('y', 12);
        noteIcon.setAttribute('text-anchor', 'end');
        noteIcon.setAttribute('font-size', '10');
        noteIcon.textContent = '📝';
        g.appendChild(noteIcon);
      }

      devicesLayer.appendChild(g);
    });
  }

  function renderConnections() {
    connectionsLayer.innerHTML = '';
    connections.forEach(conn => {
      const fromDev = findDevice(conn.from);
      const toDev = findDevice(conn.to);
      if (!fromDev || !toDev) return;

      const from = getDeviceCenter(fromDev);
      const to = getDeviceCenter(toDev);

      const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      g.setAttribute('data-conn-id', conn.id);

      const hitbox = document.createElementNS('http://www.w3.org/2000/svg', 'line');
      hitbox.setAttribute('class', 'connection-hitbox');
      hitbox.setAttribute('x1', from.x);
      hitbox.setAttribute('y1', from.y);
      hitbox.setAttribute('x2', to.x);
      hitbox.setAttribute('y2', to.y);
      g.appendChild(hitbox);

      const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
      line.setAttribute('class', 'connection-line');
      line.setAttribute('x1', from.x);
      line.setAttribute('y1', from.y);
      line.setAttribute('x2', to.x);
      line.setAttribute('y2', to.y);
      line.setAttribute('stroke', conn.color || '#4e8cff');
      line.setAttribute('stroke-width', '2.5');
      line.setAttribute('stroke-linecap', 'round');
      if (DASH_PATTERNS[conn.pattern]) {
        line.setAttribute('stroke-dasharray', DASH_PATTERNS[conn.pattern]);
      }

      if (conn.id === selectedConnectionId) {
        line.setAttribute('stroke-width', '4');
        line.setAttribute('filter', 'drop-shadow(0 0 6px ' + (conn.color || '#4e8cff') + ')');
      }

      g.appendChild(line);

      if (conn.name) {
        const midX = (from.x + to.x) / 2;
        const midY = (from.y + to.y) / 2;

        const textEl = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        textEl.setAttribute('class', 'connection-label');
        textEl.setAttribute('x', midX);
        textEl.setAttribute('y', midY - 8);
        textEl.setAttribute('text-anchor', 'middle');
        textEl.textContent = conn.name;
        g.appendChild(textEl);

        const labelBg = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        labelBg.setAttribute('class', 'connection-label-bg');
        const textLen = conn.name.length * 6.5 + 12;
        labelBg.setAttribute('x', midX - textLen / 2);
        labelBg.setAttribute('y', midY - 22);
        labelBg.setAttribute('width', textLen);
        labelBg.setAttribute('height', 18);
        labelBg.setAttribute('rx', '4');
        labelBg.setAttribute('fill', '#1a1d27');
        labelBg.setAttribute('stroke', 'rgba(255,255,255,0.06)');
        labelBg.setAttribute('stroke-width', '1');
        g.insertBefore(labelBg, textEl);
      }

      connectionsLayer.appendChild(g);
    });
  }

  // ══════════════════════════════════════════════════════
  // ── RIGHT INSPECTOR PANEL ──
  // ══════════════════════════════════════════════════════

  function updateInspector() {
    if (!selectedDeviceId) {
      inspectorEmpty.style.display = '';
      inspectorContent.style.display = 'none';
      inspPortsSection.style.display = 'none';
      return;
    }

    const device = findDevice(selectedDeviceId);
    if (!device) {
      inspectorEmpty.style.display = '';
      inspectorContent.style.display = 'none';
      inspPortsSection.style.display = 'none';
      return;
    }

    inspectorEmpty.style.display = 'none';
    inspectorContent.style.display = '';

    // Header
    inspIcon.textContent = DEVICE_TYPES[device.type]?.icon || '❓';
    inspType.textContent = DEVICE_TYPES[device.type]?.label || device.type;
    inspId.textContent = device.id;

    // Fields — only update if not currently focused
    if (document.activeElement !== inspTypeSelect) inspTypeSelect.value = device.type;
    if (document.activeElement !== inspName) inspName.value = device.label || '';
    if (document.activeElement !== inspColor) inspColor.value = device.color || '#242836';
    if (document.activeElement !== inspNotes) inspNotes.value = device.notes || '';

    // Port grid (only for switches)
    if (device.type === 'switch') {
      inspPortsSection.style.display = '';
      const pc = device.portCount || 24;
      if (document.activeElement !== inspPortCount) inspPortCount.value = String(pc);
      // Show numbering & SFP options only for 24-port
      if (pc === 24) {
        inspNumberingField.style.display = '';
        inspSfpField.style.display = '';
        if (document.activeElement !== inspPortNumbering) inspPortNumbering.value = device.portNumbering || 'ltr';
        inspSfpToggle.checked = !!device.hasSfp;
      } else {
        inspNumberingField.style.display = 'none';
        inspSfpField.style.display = 'none';
      }
      renderPortGrid(device);
    } else {
      inspPortsSection.style.display = 'none';
    }

    // Connections list
    const devConns = getDeviceConnections(device.id);
    inspConnCount.textContent = devConns.length;

    inspConnections.innerHTML = '';
    if (devConns.length === 0) {
      inspConnections.innerHTML = '<p class="no-connections">Aucune connexion</p>';
    } else {
      devConns.forEach(conn => {
        const otherId = conn.from === device.id ? conn.to : conn.from;
        const otherDev = findDevice(otherId);
        if (!otherDev) return;

        const item = document.createElement('div');
        item.className = 'conn-item';
        item.dataset.connId = conn.id;

        const colorDot = document.createElement('div');
        colorDot.className = 'conn-item-color';
        colorDot.style.background = conn.color || '#4e8cff';
        item.appendChild(colorDot);

        const info = document.createElement('div');
        info.className = 'conn-item-info';

        const target = document.createElement('div');
        target.className = 'conn-item-target';
        target.textContent = `→ ${otherDev.label}`;
        info.appendChild(target);

        if (conn.name) {
          const lbl = document.createElement('div');
          lbl.className = 'conn-item-label';
          lbl.textContent = conn.name;
          info.appendChild(lbl);
        }

        item.appendChild(info);

        const pattern = document.createElement('div');
        pattern.className = 'conn-item-pattern';
        pattern.textContent = PATTERN_LABELS[conn.pattern] || '';
        item.appendChild(pattern);

        item.addEventListener('click', () => {
          selectedConnectionId = conn.id;
          renderAll();
          const fromDev2 = findDevice(conn.from);
          const toDev2 = findDevice(conn.to);
          if (fromDev2 && toDev2) {
            const midPt = svg.createSVGPoint();
            midPt.x = (fromDev2.x + toDev2.x) / 2 + DEVICE_SIZE / 2;
            midPt.y = (fromDev2.y + toDev2.y) / 2 + DEVICE_SIZE / 2;
            const ctm = svg.getScreenCTM();
            const screenPt = midPt.matrixTransform(ctm);
            openConnectionPopover(conn.id, screenPt.x, screenPt.y);
          } else {
            openConnectionPopover(conn.id, window.innerWidth / 2, window.innerHeight / 2);
          }
        });

        inspConnections.appendChild(item);
      });
    }
  }

  // ══════════════════════════════════════════════════════
  // ── PORT GRID ──
  // ══════════════════════════════════════════════════════

  function renderPortGrid(device) {
    inspPortGrid.innerHTML = '';
    const portCount = device.portCount || 24;
    const ports = device.ports || {};

    if (portCount === 24) {
      const numbering = device.portNumbering || 'ltr';
      // 2 rows of 12
      for (let row = 0; row < 2; row++) {
        const rowEl = document.createElement('div');
        rowEl.className = 'port-row';

        const rowLabel = document.createElement('div');
        rowLabel.className = 'port-row-label';
        rowLabel.textContent = row === 0 ? '▲' : '▼';
        rowEl.appendChild(rowLabel);

        for (let col = 0; col < 12; col++) {
          let portNum;
          if (numbering === 'topdown') {
            // Top row: 1,3,5,7,9,11,13,15,17,19,21,23
            // Bottom row: 2,4,6,8,10,12,14,16,18,20,22,24
            portNum = col * 2 + row + 1;
          } else {
            // LTR: Top row 1-12, Bottom row 13-24
            portNum = row * 12 + col + 1;
          }
          rowEl.appendChild(createPortSlot(portNum, ports, device, false));
        }
        inspPortGrid.appendChild(rowEl);
      }
      // SFP port
      if (device.hasSfp) {
        const sfpRow = document.createElement('div');
        sfpRow.className = 'port-row';
        const sfpLabel = document.createElement('div');
        sfpLabel.className = 'port-row-label';
        sfpLabel.textContent = '⬥';
        sfpRow.appendChild(sfpLabel);
        sfpRow.appendChild(createPortSlot(25, ports, device, true));
        inspPortGrid.appendChild(sfpRow);
      }
    } else {
      // 1 row of 5
      const rowEl = document.createElement('div');
      rowEl.className = 'port-row';

      const rowLabel = document.createElement('div');
      rowLabel.className = 'port-row-label';
      rowLabel.textContent = '';
      rowEl.appendChild(rowLabel);

      for (let col = 0; col < 5; col++) {
        const portNum = col + 1;
        rowEl.appendChild(createPortSlot(portNum, ports, device));
      }
      inspPortGrid.appendChild(rowEl);
    }
  }

  // Helper: normalize port data (backward compat: string → object)
  function getPortData(ports, portNum) {
    const entry = ports[portNum];
    if (!entry) return null;
    if (typeof entry === 'string') return { deviceId: entry, color: '', vlan: '' };
    return entry;
  }

  function createPortSlot(portNum, ports, device, isSfp) {
    const slot = document.createElement('div');
    slot.className = 'port-slot';
    if (isSfp) slot.classList.add('sfp-port');
    slot.dataset.port = portNum;

    const portData = getPortData(ports, portNum);
    const assignedDevice = portData?.deviceId ? findDevice(portData.deviceId) : null;

    if (assignedDevice || portData?.vlan) {
      slot.classList.add('occupied');
    }

    if (portData?.color) {
      slot.style.borderColor = portData.color;
      slot.style.boxShadow = `0 0 4px ${portData.color}40`;
    }

    if (selectedPortNumber === portNum) {
      slot.classList.add('selected');
    }

    const numEl = document.createElement('div');
    numEl.className = 'port-number';
    numEl.textContent = isSfp ? 'SFP' : portNum;
    slot.appendChild(numEl);

    if (assignedDevice) {
      const deviceLabel = document.createElement('div');
      deviceLabel.className = 'port-device-label';
      deviceLabel.textContent = assignedDevice.label;
      slot.appendChild(deviceLabel);
    }

    if (portData?.vlan) {
      const vlanEl = document.createElement('div');
      vlanEl.className = 'port-vlan-label';
      vlanEl.textContent = `V${portData.vlan}`;
      slot.appendChild(vlanEl);
    }

    // Tooltip
    const parts = [`Port ${isSfp ? 'SFP' : portNum}`];
    if (assignedDevice) parts.push(assignedDevice.label);
    if (portData?.vlan) parts.push(`VLAN ${portData.vlan}`);
    slot.title = parts.join(' — ');

    slot.addEventListener('click', () => {
      selectedPortNumber = portNum;
      openPortAssignment(device, portNum);
      renderPortGrid(device);
    });

    return slot;
  }

  function openPortAssignment(device, portNum) {
    portAssignPanel.style.display = '';
    portAssignNumber.textContent = `#${portNum}`;

    // Build dropdown: all devices
    const devConns = getDeviceConnections(device.id);
    const connectedDeviceIds = new Set();
    devConns.forEach(c => {
      if (c.from === device.id) connectedDeviceIds.add(c.to);
      if (c.to === device.id) connectedDeviceIds.add(c.from);
    });

    portAssignSelect.innerHTML = '<option value="">— Vide —</option>';

    if (connectedDeviceIds.size > 0) {
      const optGroup1 = document.createElement('optgroup');
      optGroup1.label = '— Connectés —';
      connectedDeviceIds.forEach(devId => {
        const d = findDevice(devId);
        if (!d) return;
        const opt = document.createElement('option');
        opt.value = d.id;
        opt.textContent = `${DEVICE_TYPES[d.type]?.icon || ''} ${d.label}`;
        optGroup1.appendChild(opt);
      });
      portAssignSelect.appendChild(optGroup1);
    }

    const otherDevices = devices.filter(d => d.id !== device.id && !connectedDeviceIds.has(d.id));
    if (otherDevices.length > 0) {
      const optGroup2 = document.createElement('optgroup');
      optGroup2.label = '— Autres —';
      otherDevices.forEach(d => {
        const opt = document.createElement('option');
        opt.value = d.id;
        opt.textContent = `${DEVICE_TYPES[d.type]?.icon || ''} ${d.label}`;
        optGroup2.appendChild(opt);
      });
      portAssignSelect.appendChild(optGroup2);
    }

    // Pre-fill current values
    const portData = getPortData(device.ports || {}, portNum);
    portAssignSelect.value = portData?.deviceId || '';
    portAssignVlan.value = portData?.vlan || '';
    portAssignColor.value = portData?.color || '#4e8cff';
  }

  function closePortAssignment() {
    portAssignPanel.style.display = 'none';
    selectedPortNumber = null;
    const device = findDevice(selectedDeviceId);
    if (device) renderPortGrid(device);
  }

  portAssignCancel.addEventListener('click', closePortAssignment);

  portAssignOk.addEventListener('click', () => {
    const device = findDevice(selectedDeviceId);
    if (!device || selectedPortNumber === null) return;

    if (!device.ports) device.ports = {};

    const deviceId = portAssignSelect.value;
    const vlan = portAssignVlan.value.trim();
    const color = portAssignColor.value;

    if (deviceId || vlan || color !== '#4e8cff') {
      device.ports[selectedPortNumber] = { deviceId, color, vlan };
    } else {
      delete device.ports[selectedPortNumber];
    }

    closePortAssignment();
    showToast(`Port #${selectedPortNumber} mis à jour`, 'success');
  });

  // Port color presets
  portColorPresets.addEventListener('click', (e) => {
    const preset = e.target.closest('.color-preset');
    if (!preset) return;
    portAssignColor.value = preset.dataset.color;
  });

  // Port count change
  inspPortCount.addEventListener('change', () => {
    const device = findDevice(selectedDeviceId);
    if (!device) return;
    device.portCount = parseInt(inspPortCount.value);
    device.ports = {}; // Reset assignments when changing port count
    closePortAssignment();
    renderPortGrid(device);
    updateInspector();
    showToast(`Configuration changée: ${device.portCount} ports`, 'info');
  });

  // Port numbering change
  inspPortNumbering.addEventListener('change', () => {
    const device = findDevice(selectedDeviceId);
    if (!device) return;
    device.portNumbering = inspPortNumbering.value;
    renderPortGrid(device);
    updateInspector();
    showToast(`Numérotation: ${inspPortNumbering.value === 'topdown' ? 'Haut-bas' : 'Gauche à droite'}`, 'info');
  });

  // SFP toggle
  inspSfpToggle.addEventListener('change', () => {
    const device = findDevice(selectedDeviceId);
    if (!device) return;
    device.hasSfp = inspSfpToggle.checked;
    renderPortGrid(device);
    showToast(device.hasSfp ? 'Port SFP activé' : 'Port SFP désactivé', 'info');
  });

  // Inspector color presets
  inspColorPresets.addEventListener('click', (e) => {
    const preset = e.target.closest('.color-preset');
    if (!preset) return;
    inspColor.value = preset.dataset.color;
  });

  // Apply inspector changes
  inspApply.addEventListener('click', () => applyInspectorChanges());

  function applyInspectorChanges() {
    const device = findDevice(selectedDeviceId);
    if (!device) return;

    const newType = inspTypeSelect.value;
    if (newType && newType !== device.type) {
      device.type = newType;
      if (newType === 'switch') {
        device.portCount = device.portCount || 24;
        device.ports = device.ports || {};
      }
    }

    const newName = inspName.value.trim();
    if (newName) device.label = newName;
    device.color = inspColor.value === '#242836' ? '' : inspColor.value;
    device.notes = inspNotes.value.trim();

    renderAll();
    showToast('Appareil mis à jour ✓', 'success');
  }

  // Delete device from inspector
  inspDeleteDevice.addEventListener('click', () => {
    if (!selectedDeviceId) return;
    const dev = findDevice(selectedDeviceId);
    devices = devices.filter(d => d.id !== selectedDeviceId);
    connections = connections.filter(c => c.from !== selectedDeviceId && c.to !== selectedDeviceId);
    // Also remove from other switches port assignments
    devices.forEach(d => {
      if (d.ports) {
        Object.keys(d.ports).forEach(key => {
          const pd = getPortData(d.ports, key);
          if (pd?.deviceId === selectedDeviceId) delete d.ports[key];
        });
      }
    });
    selectedDeviceId = null;
    renderAll();
    showToast(`${dev?.label || 'Appareil'} supprimé`, 'success');
  });

  // Auto-apply on field change
  inspTypeSelect.addEventListener('change', applyInspectorChanges);
  inspName.addEventListener('change', applyInspectorChanges);
  inspColor.addEventListener('change', applyInspectorChanges);
  inspNotes.addEventListener('change', applyInspectorChanges);

  inspName.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      applyInspectorChanges();
      inspName.blur();
    }
  });

  // ── Palette Click ──
  palette.addEventListener('click', (e) => {
    const item = e.target.closest('.palette-item');
    if (!item) return;

    const type = item.dataset.type;
    if (currentMode === 'place' && selectedDeviceType === type) {
      setMode('select');
    } else {
      selectedDeviceType = type;
      setMode('place');
    }
    clearConnectState();
  });

  // ── SVG Canvas Events ──
  svg.addEventListener('mousedown', (e) => {
    const svgPt = getSVGPoint(e.clientX, e.clientY);
    const deviceEl = e.target.closest('.device-group');
    const connEl = e.target.closest('[data-conn-id]');

    if (currentMode === 'connect') {
      if (deviceEl) {
        const id = deviceEl.getAttribute('data-id');
        if (!connectSourceId) {
          connectSourceId = id;
          selectedDeviceId = id;
          renderDevices();
          updateInspector();
          showStatus('Cliquez sur l\'appareil cible pour créer la connexion');
        } else if (id !== connectSourceId) {
          createConnection(connectSourceId, id);
          connectSourceId = null;
          selectedDeviceId = id;
          renderAll();
          showStatus('Connexion créée ✓');
        }
      }
      return;
    }

    if (deviceEl) {
      const id = deviceEl.getAttribute('data-id');
      const device = findDevice(id);

      if (e.shiftKey) {
        // Shift+click: toggle in group selection
        if (selectedDeviceIds.has(id)) {
          selectedDeviceIds.delete(id);
          if (selectedDeviceId === id) selectedDeviceId = null;
        } else {
          selectedDeviceIds.add(id);
          if (selectedDeviceId) selectedDeviceIds.add(selectedDeviceId);
          selectedDeviceId = id;
        }
      } else if (!selectedDeviceIds.has(id)) {
        // Normal click: select only this one
        selectedDeviceIds.clear();
        selectedDeviceId = id;
      }
      // If already in group and no shift, keep group (for dragging)
      if (!e.shiftKey && selectedDeviceIds.size > 0 && selectedDeviceIds.has(id)) {
        selectedDeviceId = id;
      }

      selectedConnectionId = null;
      closePortAssignment();
      renderAll();

      if (device) {
        isDragging = true;
        dragDeviceId = id;
        dragOffsetX = svgPt.x - device.x;
        dragOffsetY = svgPt.y - device.y;
      }
      return;
    }

    if (connEl) {
      const connId = connEl.getAttribute('data-conn-id');
      selectedConnectionId = connId;
      selectedDeviceId = null;
      selectedDeviceIds.clear();
      renderAll();
      openConnectionPopover(connId, e.clientX, e.clientY);
      return;
    }

    if (currentMode === 'place' && selectedDeviceType) {
      placeDevice(selectedDeviceType, svgPt.x - DEVICE_SIZE / 2, svgPt.y - DEVICE_SIZE / 2);
      renderAll();
      return;
    }

    // Start rubber band or pan on empty canvas
    if (currentMode === 'select') {
      if (isSpaceHeld) {
        // Space held = pan
        isPanning = true;
        panStartX = e.clientX;
        panStartY = e.clientY;
        panStartVBX = viewBox.x;
        panStartVBY = viewBox.y;
      } else {
        // Rubber band selection
        isRubberBanding = true;
        rubberBandStart = { x: svgPt.x, y: svgPt.y };
        selectedDeviceId = null;
        selectedDeviceIds.clear();
        selectedConnectionId = null;
        closePortAssignment();
        renderAll();
      }
      return;
    }

    // Fallback: pan
    selectedDeviceId = null;
    selectedDeviceIds.clear();
    selectedConnectionId = null;
    closePortAssignment();
    renderAll();

    isPanning = true;
    panStartX = e.clientX;
    panStartY = e.clientY;
    panStartVBX = viewBox.x;
    panStartVBY = viewBox.y;
  });

  svg.addEventListener('mousemove', (e) => {
    if (isDragging && dragDeviceId) {
      const svgPt = getSVGPoint(e.clientX, e.clientY);
      const device = findDevice(dragDeviceId);
      if (device) {
        const dx = svgPt.x - dragOffsetX - device.x;
        const dy = svgPt.y - dragOffsetY - device.y;
        // Move all selected devices together
        if (selectedDeviceIds.size > 0 && selectedDeviceIds.has(dragDeviceId)) {
          selectedDeviceIds.forEach(id => {
            const d = findDevice(id);
            if (d) { d.x += dx; d.y += dy; }
          });
        } else {
          device.x += dx;
          device.y += dy;
        }
        dragOffsetX = svgPt.x - device.x;
        dragOffsetY = svgPt.y - device.y;
        renderConnections();
        renderDevices();
      }
      return;
    }

    if (isRubberBanding && rubberBandStart) {
      const svgPt = getSVGPoint(e.clientX, e.clientY);
      const x = Math.min(rubberBandStart.x, svgPt.x);
      const y = Math.min(rubberBandStart.y, svgPt.y);
      const w = Math.abs(svgPt.x - rubberBandStart.x);
      const h = Math.abs(svgPt.y - rubberBandStart.y);
      rubberBandRect = { x, y, w, h };

      // Draw rubber band
      tempConnLayer.innerHTML = '';
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.setAttribute('x', x);
      rect.setAttribute('y', y);
      rect.setAttribute('width', w);
      rect.setAttribute('height', h);
      rect.setAttribute('fill', 'rgba(78, 140, 255, 0.08)');
      rect.setAttribute('stroke', '#4e8cff');
      rect.setAttribute('stroke-width', '1');
      rect.setAttribute('stroke-dasharray', '6 3');
      rect.setAttribute('rx', '4');
      tempConnLayer.appendChild(rect);

      // Live highlight devices inside rect
      selectedDeviceIds.clear();
      devices.forEach(d => {
        const cx = d.x + DEVICE_SIZE / 2;
        const cy = d.y + DEVICE_SIZE / 2;
        if (cx >= x && cx <= x + w && cy >= y && cy <= y + h) {
          selectedDeviceIds.add(d.id);
        }
      });
      renderDevices();
      return;
    }

    if (isPanning) {
      const dx = (e.clientX - panStartX) * (viewBox.w / svg.getBoundingClientRect().width);
      const dy = (e.clientY - panStartY) * (viewBox.h / svg.getBoundingClientRect().height);
      viewBox.x = panStartVBX - dx;
      viewBox.y = panStartVBY - dy;
      applyViewBox();
      return;
    }

    if (currentMode === 'connect' && connectSourceId) {
      const sourceDev = findDevice(connectSourceId);
      if (sourceDev) {
        const svgPt = getSVGPoint(e.clientX, e.clientY);
        const from = getDeviceCenter(sourceDev);
        tempConnLayer.innerHTML = '';
        const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        line.setAttribute('class', 'temp-connection');
        line.setAttribute('x1', from.x);
        line.setAttribute('y1', from.y);
        line.setAttribute('x2', svgPt.x);
        line.setAttribute('y2', svgPt.y);
        tempConnLayer.appendChild(line);
      }
    }
  });

  svg.addEventListener('mouseup', () => {
    if (isRubberBanding) {
      isRubberBanding = false;
      rubberBandStart = null;
      rubberBandRect = null;
      tempConnLayer.innerHTML = '';
      if (selectedDeviceIds.size === 1) {
        selectedDeviceId = [...selectedDeviceIds][0];
      } else if (selectedDeviceIds.size > 1) {
        selectedDeviceId = [...selectedDeviceIds][0];
      }
      renderAll();
      if (selectedDeviceIds.size > 0) {
        showStatus(`${selectedDeviceIds.size} appareil(s) sélectionné(s)`);
      }
    }
    isDragging = false;
    dragDeviceId = null;
    isPanning = false;
  });

  svg.addEventListener('mouseleave', () => {
    isDragging = false;
    dragDeviceId = null;
    isPanning = false;
    if (isRubberBanding) {
      isRubberBanding = false;
      tempConnLayer.innerHTML = '';
    }
  });

  // ── Zoom ──
  svg.addEventListener('wheel', (e) => {
    e.preventDefault();
    zoom(e.deltaY > 0 ? 1.08 : 0.92, e.clientX, e.clientY);
  });

  function zoom(factor, cx, cy) {
    const svgRect = svg.getBoundingClientRect();
    const mx = (cx - svgRect.left) / svgRect.width;
    const my = (cy - svgRect.top) / svgRect.height;
    const newW = viewBox.w * factor;
    const newH = viewBox.h * factor;
    viewBox.x += (viewBox.w - newW) * mx;
    viewBox.y += (viewBox.h - newH) * my;
    viewBox.w = newW;
    viewBox.h = newH;
    zoomLevel /= factor;
    applyViewBox();
  }

  btnZoomIn.addEventListener('click', () => {
    const r = svg.getBoundingClientRect();
    zoom(0.8, r.left + r.width / 2, r.top + r.height / 2);
  });
  btnZoomOut.addEventListener('click', () => {
    const r = svg.getBoundingClientRect();
    zoom(1.25, r.left + r.width / 2, r.top + r.height / 2);
  });
  btnZoomReset.addEventListener('click', () => { zoomLevel = 1; initViewBox(); });

  // ── Device Placement ──
  function placeDevice(type, x, y) {
    const id = uuid();
    const label = DEVICE_TYPES[type]?.label || type;
    const device = { id, type, x, y, label, color: '', notes: '' };
    if (type === 'switch') {
      device.portCount = 24;
      device.ports = {};
    }
    devices.push(device);
    selectedDeviceId = id;
    showStatus(`${label} placé ✓`);
  }

  // ── Connection Creation ──
  function createConnection(fromId, toId) {
    const exists = connections.find(c =>
      (c.from === fromId && c.to === toId) ||
      (c.from === toId && c.to === fromId)
    );
    if (exists) {
      showToast('Cette connexion existe déjà', 'error');
      return;
    }
    connections.push({
      id: connUuid(),
      from: fromId,
      to: toId,
      name: '',
      color: '#4e8cff',
      pattern: 'solid',
    });
  }

  function clearConnectState() {
    connectSourceId = null;
    tempConnLayer.innerHTML = '';
  }

  // ── Connect Mode Toggle ──
  btnConnect.addEventListener('click', () => {
    if (currentMode === 'connect') {
      setMode('select');
      clearConnectState();
    } else {
      setMode('connect');
      showStatus('Mode connexion: cliquez sur l\'appareil source');
    }
  });

  // ── Delete ──
  btnDelete.addEventListener('click', () => {
    if (selectedDeviceId) {
      const dev = findDevice(selectedDeviceId);
      devices = devices.filter(d => d.id !== selectedDeviceId);
      connections = connections.filter(c => c.from !== selectedDeviceId && c.to !== selectedDeviceId);
      devices.forEach(d => {
        if (d.ports) {
          Object.keys(d.ports).forEach(key => {
            if (d.ports[key] === selectedDeviceId) delete d.ports[key];
          });
        }
      });
      selectedDeviceId = null;
      renderAll();
      showToast(`${dev?.label || 'Appareil'} supprimé`, 'success');
    } else if (selectedConnectionId) {
      connections = connections.filter(c => c.id !== selectedConnectionId);
      selectedConnectionId = null;
      renderAll();
      showToast('Connexion supprimée', 'success');
    } else {
      showToast('Rien de sélectionné', 'error');
    }
  });

  // ── Clear All ──
  btnClear.addEventListener('click', () => {
    if (devices.length === 0 && connections.length === 0) return;
    if (!confirm('Voulez-vous vraiment tout effacer ?')) return;
    devices = [];
    connections = [];
    selectedDeviceId = null;
    selectedConnectionId = null;
    nextDeviceId = 1;
    nextConnId = 1;
    renderAll();
    showToast('Canvas vidé', 'info');
  });

  // ── Keyboard shortcuts ──
  document.addEventListener('keydown', (e) => {
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA' || e.target.tagName === 'SELECT') return;

    if (e.key === 'Delete' || e.key === 'Backspace') {
      btnDelete.click();
    } else if (e.key === 'Escape') {
      setMode('select');
      clearConnectState();
      closePopover();
      closePortAssignment();
      selectedDeviceId = null;
      selectedDeviceIds.clear();
      selectedConnectionId = null;
      renderAll();
    } else if (e.key === 'c' || e.key === 'C') {
      btnConnect.click();
    } else if (e.key === ' ') {
      e.preventDefault(); // prevent page scroll
      if (!isSpaceHeld) {
        isSpaceHeld = true;
        svg.style.cursor = 'grab';
      }
    }
  });

  document.addEventListener('keyup', (e) => {
    if (e.key === ' ') {
      isSpaceHeld = false;
      svg.style.cursor = '';
    }
  });

  // ── Connection Popover ──
  function openConnectionPopover(connId, clientX, clientY) {
    const conn = findConnection(connId);
    if (!conn) return;

    connNameInput.value = conn.name || '';
    connColorInput.value = conn.color || '#4e8cff';
    connPatternSelect.value = conn.pattern || 'solid';

    const x = Math.min(clientX + 10, window.innerWidth - 600);
    const y = Math.min(clientY + 10, window.innerHeight - 340);
    popover.style.left = x + 'px';
    popover.style.top = y + 'px';

    popover.classList.add('visible');
    popoverOverlay.classList.add('visible');
    popover._connId = connId;

    setTimeout(() => connNameInput.focus(), 100);
  }

  function closePopover() {
    popover.classList.remove('visible');
    popoverOverlay.classList.remove('visible');
    popover._connId = null;
  }

  popoverClose.addEventListener('click', closePopover);
  popoverOverlay.addEventListener('click', closePopover);

  colorPresets.addEventListener('click', (e) => {
    const preset = e.target.closest('.color-preset');
    if (!preset) return;
    connColorInput.value = preset.dataset.color;
  });

  connSaveBtn.addEventListener('click', () => {
    const conn = findConnection(popover._connId);
    if (!conn) return;
    conn.name = connNameInput.value.trim();
    conn.color = connColorInput.value;
    conn.pattern = connPatternSelect.value;
    renderAll();
    closePopover();
    showToast('Lien mis à jour', 'success');
  });

  connDeleteBtn.addEventListener('click', () => {
    connections = connections.filter(c => c.id !== popover._connId);
    selectedConnectionId = null;
    renderAll();
    closePopover();
    showToast('Connexion supprimée', 'success');
  });

  // ── Save / Load ──
  btnSave.addEventListener('click', () => {
    const data = {
      version: 2,
      timestamp: new Date().toISOString(),
      viewBox: { ...viewBox },
      zoomLevel,
      devices: devices.map(d => {
        const out = { ...d };
        if (d.type === 'switch') {
          out.portCount = d.portCount || 24;
          out.ports = d.ports ? { ...d.ports } : {};
          out.portNumbering = d.portNumbering || 'ltr';
          out.hasSfp = !!d.hasSfp;
        }
        return out;
      }),
      connections: connections.map(c => ({ ...c })),
      nextDeviceId,
      nextConnId,
    };

    const json = JSON.stringify(data, null, 2);
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `plantation-reseau-${new Date().toISOString().slice(0, 10)}.json`;
    a.click();
    URL.revokeObjectURL(url);
    showToast('Configuration sauvegardée ✓', 'success');
  });

  btnLoad.addEventListener('click', () => fileInput.click());

  fileInput.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (ev) => {
      try {
        loadConfig(JSON.parse(ev.target.result));
        showToast('Configuration chargée ✓', 'success');
      } catch (err) {
        showToast('Erreur: fichier invalide', 'error');
        console.error(err);
      }
    };
    reader.readAsText(file);
    fileInput.value = '';
  });

  function loadConfig(data) {
    devices = (data.devices || []).map(d => {
      // Ensure switches have ports data
      if (d.type === 'switch') {
        d.portCount = d.portCount || 24;
        d.ports = d.ports || {};
      }
      return d;
    });
    connections = data.connections || [];
    nextDeviceId = data.nextDeviceId || devices.length + 1;
    nextConnId = data.nextConnId || connections.length + 1;

    if (data.viewBox) {
      viewBox.x = data.viewBox.x;
      viewBox.y = data.viewBox.y;
      viewBox.w = data.viewBox.w;
      viewBox.h = data.viewBox.h;
      applyViewBox();
    }
    if (data.zoomLevel) zoomLevel = data.zoomLevel;

    selectedDeviceId = null;
    selectedConnectionId = null;
    setMode('select');
    renderAll();
  }

  // ══════════════════════════════════════════════════════
  // ── PDF EXPORT ──
  // ══════════════════════════════════════════════════════

  btnExport.addEventListener('click', async () => {
    showToast('Génération du PDF en cours…', 'info');

    try {
      const prevSelected = selectedDeviceId;
      const prevConnSelected = selectedConnectionId;
      selectedDeviceId = null;
      selectedConnectionId = null;
      selectedDeviceIds.clear();
      renderAll();

      // Compute tight bounding box of all devices
      let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
      devices.forEach(d => {
        minX = Math.min(minX, d.x);
        minY = Math.min(minY, d.y);
        maxX = Math.max(maxX, d.x + DEVICE_SIZE);
        maxY = Math.max(maxY, d.y + DEVICE_SIZE + 20); // +20 for label
      });

      // Fallback if no devices
      if (!isFinite(minX)) {
        minX = viewBox.x; minY = viewBox.y;
        maxX = viewBox.x + viewBox.w; maxY = viewBox.y + viewBox.h;
      }

      // Add padding (10% of content size, min 40px)
      const padX = Math.max(40, (maxX - minX) * 0.1);
      const padY = Math.max(40, (maxY - minY) * 0.1);
      const contentVB = {
        x: minX - padX,
        y: minY - padY,
        w: (maxX - minX) + padX * 2,
        h: (maxY - minY) + padY * 2,
      };

      // Use landscape aspect ratio for the render
      const renderW = 1600;
      const renderH = renderW * (contentVB.h / contentVB.w);

      const container = document.createElement('div');
      container.style.position = 'fixed';
      container.style.top = '0';
      container.style.left = '0';
      container.style.width = renderW + 'px';
      container.style.height = renderH + 'px';
      container.style.zIndex = '-999';
      container.style.background = '#0f1117';
      container.style.overflow = 'hidden';

      const svgClone = svg.cloneNode(true);
      svgClone.setAttribute('viewBox', `${contentVB.x} ${contentVB.y} ${contentVB.w} ${contentVB.h}`);
      svgClone.style.width = '100%';
      svgClone.style.height = '100%';
      container.appendChild(svgClone);
      document.body.appendChild(container);

      const canvas = await html2canvas(container, {
        backgroundColor: '#0f1117',
        scale: 2,
        useCORS: true,
        logging: false,
      });

      document.body.removeChild(container);

      const { jsPDF } = window.jspdf;
      const pdf = new jsPDF({ orientation: 'landscape', unit: 'mm', format: 'a4' });
      const pageW = pdf.internal.pageSize.getWidth();
      const pageH = pdf.internal.pageSize.getHeight();

      // ── Page 1: Diagram ──
      pdf.setFontSize(20);
      pdf.setTextColor(30, 30, 40);
      pdf.text('Plan de Plantation Réseau', pageW / 2, 15, { align: 'center' });

      pdf.setFontSize(10);
      pdf.setTextColor(120, 120, 130);
      pdf.text(`Généré le ${new Date().toLocaleDateString('fr-CA')} à ${new Date().toLocaleTimeString('fr-CA')}`, pageW / 2, 22, { align: 'center' });

      const imgData = canvas.toDataURL('image/png');
      const imgW = pageW - 20;
      const imgH = (canvas.height / canvas.width) * imgW;
      const maxImgH = pageH - 35;
      const finalImgH = Math.min(imgH, maxImgH);
      const finalImgW = (finalImgH / imgH) * imgW;
      pdf.addImage(imgData, 'PNG', (pageW - finalImgW) / 2, 27, finalImgW, finalImgH);

      // ── Page 2+: Technical tables ──
      if (connections.length > 0 || devices.length > 0) {
        pdf.addPage();
        let y = 15;

        // Connection table
        if (connections.length > 0) {
          pdf.setFontSize(16);
          pdf.setTextColor(30, 30, 40);
          pdf.text('Liste Technique des Connexions', pageW / 2, y, { align: 'center' });
          y += 13;

          const cols = [15, 55, 95, 150, 200, 240];
          const colLabels = ['#', 'Source', 'Destination', 'Nom du lien', 'Style', 'Couleur'];

          pdf.setFontSize(9);
          pdf.setTextColor(255, 255, 255);
          pdf.setFillColor(36, 40, 54);
          pdf.rect(10, y - 5, pageW - 20, 8, 'F');
          colLabels.forEach((label, i) => pdf.text(label, cols[i], y));
          y += 10;

          connections.forEach((conn, idx) => {
            const fromDev = findDevice(conn.from);
            const toDev = findDevice(conn.to);
            if (!fromDev || !toDev) return;

            if (y > pageH - 15) { pdf.addPage(); y = 20; }

            if (idx % 2 === 0) {
              pdf.setFillColor(245, 245, 248);
              pdf.rect(10, y - 5, pageW - 20, 8, 'F');
            }

            pdf.setTextColor(60, 60, 70);
            pdf.setFontSize(9);
            pdf.text(`${idx + 1}`, cols[0], y);
            pdf.text(fromDev.label, cols[1], y);
            pdf.text(toDev.label, cols[2], y);
            pdf.text(conn.name || '—', cols[3], y);
            pdf.text(PATTERN_LABELS[conn.pattern] || conn.pattern, cols[4], y);

            const hex = conn.color || '#4e8cff';
            const r = parseInt(hex.slice(1, 3), 16);
            const g = parseInt(hex.slice(3, 5), 16);
            const b = parseInt(hex.slice(5, 7), 16);
            pdf.setFillColor(r, g, b);
            pdf.circle(cols[5] + 3, y - 2, 3, 'F');

            y += 10;
          });

          y += 10;
        }

        // Device inventory with better notes spacing
        if (y > pageH - 50) { pdf.addPage(); y = 15; }

        pdf.setFontSize(16);
        pdf.setTextColor(30, 30, 40);
        pdf.text('Inventaire des Équipements', pageW / 2, y, { align: 'center' });
        y += 12;

        pdf.setFontSize(9);
        pdf.setTextColor(255, 255, 255);
        pdf.setFillColor(36, 40, 54);
        pdf.rect(10, y - 5, pageW - 20, 8, 'F');
        pdf.text('#', 15, y);
        pdf.text('Type', 30, y);
        pdf.text('Nom', 90, y);
        pdf.text('Notes', 155, y);
        y += 10;

        devices.forEach((dev, idx) => {
          // Calculate row height based on notes content
          const notesText = dev.notes || '—';
          const notesLines = pdf.splitTextToSize(notesText, pageW - 170);
          const rowHeight = Math.max(8, notesLines.length * 5 + 3);

          if (y + rowHeight > pageH - 15) { pdf.addPage(); y = 20; }

          if (idx % 2 === 0) {
            pdf.setFillColor(245, 245, 248);
            pdf.rect(10, y - 5, pageW - 20, rowHeight, 'F');
          }

          pdf.setTextColor(60, 60, 70);
          pdf.setFontSize(9);
          pdf.text(`${idx + 1}`, 15, y);
          pdf.text(DEVICE_TYPES[dev.type]?.label || dev.type, 30, y);
          pdf.text(dev.label, 90, y);

          // Multi-line notes
          pdf.setFontSize(8);
          pdf.setTextColor(80, 80, 90);
          notesLines.forEach((line, lineIdx) => {
            pdf.text(line, 155, y + (lineIdx * 5));
          });

          y += rowHeight;
        });

        // ── Port Assignments Table ──
        const switches = devices.filter(d => d.type === 'switch' && d.ports && Object.keys(d.ports).length > 0);
        if (switches.length > 0) {
          if (y > pageH - 60) { pdf.addPage(); y = 15; }
          y += 5;

          pdf.setFontSize(16);
          pdf.setTextColor(30, 30, 40);
          pdf.text('Affectation des Ports', pageW / 2, y, { align: 'center' });
          y += 12;

          switches.forEach(sw => {
            if (y > pageH - 50) { pdf.addPage(); y = 15; }

            // Switch title
            pdf.setFontSize(12);
            pdf.setTextColor(30, 30, 40);
            const sfpLabel = sw.hasSfp ? ' + SFP' : '';
            const numLabel = sw.portNumbering === 'topdown' ? ' (Haut-bas)' : '';
            pdf.text(`${sw.label} (${sw.portCount || 24} ports${sfpLabel}${numLabel})`, 15, y);
            y += 8;

            // Port grid visual
            const portCount = sw.portCount || 24;
            const ports = sw.ports || {};
            const numbering = sw.portNumbering || 'ltr';
            const portsPerRow = portCount === 5 ? 5 : 12;
            const rows = portCount === 5 ? 1 : 2;
            const cellW = Math.min(20, (pageW - 40) / portsPerRow);
            const cellH = 12;
            const gridStartX = 15;

            for (let row = 0; row < rows; row++) {
              for (let col = 0; col < portsPerRow; col++) {
                let portNum;
                if (portCount === 24 && numbering === 'topdown') {
                  portNum = col * 2 + row + 1;
                } else {
                  portNum = row * portsPerRow + col + 1;
                }
                if (portNum > portCount) break;

                const cx = gridStartX + col * (cellW + 1);
                const cy = y;
                const pd = getPortData(ports, portNum);
                const assignedDev = pd?.deviceId ? findDevice(pd.deviceId) : null;

                // Cell background with port color
                if (pd?.color && pd.color !== '#4e8cff') {
                  const r = parseInt(pd.color.slice(1, 3), 16);
                  const g = parseInt(pd.color.slice(3, 5), 16);
                  const b = parseInt(pd.color.slice(5, 7), 16);
                  pdf.setFillColor(r, g, b);
                } else if (assignedDev) {
                  pdf.setFillColor(60, 100, 180);
                } else {
                  pdf.setFillColor(230, 232, 236);
                }
                pdf.rect(cx, cy, cellW, cellH, 'F');

                // Port number
                const hasContent = assignedDev || pd?.vlan;
                pdf.setFontSize(6);
                pdf.setTextColor(hasContent ? 255 : 140, hasContent ? 255 : 140, hasContent ? 255 : 150);
                pdf.text(`${portNum}`, cx + 1, cy + 4);

                // Device name
                if (assignedDev) {
                  pdf.setFontSize(5);
                  pdf.setTextColor(255, 255, 255);
                  const truncName = assignedDev.label.length > 8 ? assignedDev.label.substring(0, 7) + '…' : assignedDev.label;
                  pdf.text(truncName, cx + 1, cy + 8);
                }

                // VLAN
                if (pd?.vlan) {
                  pdf.setFontSize(4);
                  pdf.setTextColor(255, 255, 200);
                  pdf.text(pd.vlan.length > 6 ? pd.vlan.substring(0, 5) + '…' : pd.vlan, cx + 1, cy + 11);
                }
              }
              y += cellH + 2;
            }

            // SFP port row
            if (sw.hasSfp && portCount === 24) {
              const sfpPd = getPortData(ports, 25);
              const sfpDev = sfpPd?.deviceId ? findDevice(sfpPd.deviceId) : null;
              const cx = gridStartX;
              const cy = y;

              if (sfpPd?.color && sfpPd.color !== '#4e8cff') {
                const r = parseInt(sfpPd.color.slice(1, 3), 16);
                const g = parseInt(sfpPd.color.slice(3, 5), 16);
                const b = parseInt(sfpPd.color.slice(5, 7), 16);
                pdf.setFillColor(r, g, b);
              } else if (sfpDev) {
                pdf.setFillColor(60, 100, 180);
              } else {
                pdf.setFillColor(230, 232, 236);
              }
              pdf.rect(cx, cy, cellW * 2, cellH, 'F');
              pdf.setDrawColor(200, 140, 60);
              pdf.rect(cx, cy, cellW * 2, cellH, 'S');

              pdf.setFontSize(6);
              pdf.setTextColor(sfpDev ? 255 : 140, sfpDev ? 255 : 140, sfpDev ? 255 : 150);
              pdf.text('SFP', cx + 1, cy + 4);

              if (sfpDev) {
                pdf.setFontSize(5);
                pdf.setTextColor(255, 255, 255);
                pdf.text(sfpDev.label, cx + 1, cy + 8);
              }
              if (sfpPd?.vlan) {
                pdf.setFontSize(4);
                pdf.setTextColor(255, 255, 200);
                pdf.text(sfpPd.vlan, cx + 1, cy + 11);
              }
              y += cellH + 2;
            }

            // Legend table below grid
            const usedPorts = Object.entries(ports)
              .map(([pNum, entry]) => {
                const pd = typeof entry === 'string' ? { deviceId: entry, color: '', vlan: '' } : entry;
                return { portNum: parseInt(pNum), ...pd };
              })
              .filter(p => p.deviceId || p.vlan);

            if (usedPorts.length > 0) {
              y += 2;
              pdf.setFontSize(7);
              pdf.setTextColor(100, 100, 110);
              pdf.text('Port', 20, y);
              pdf.text('Appareil', 40, y);
              pdf.text('VLAN', 90, y);
              pdf.text('Type', 115, y);
              pdf.text('Notes', 155, y);
              y += 5;

              usedPorts.sort((a, b) => a.portNum - b.portNum);
              usedPorts.forEach(p => {
                if (y > pageH - 15) { pdf.addPage(); y = 20; }
                const dev = p.deviceId ? findDevice(p.deviceId) : null;

                pdf.setFontSize(7);
                pdf.setTextColor(60, 60, 70);
                pdf.text(`#${p.portNum === 25 ? 'SFP' : p.portNum}`, 20, y);
                pdf.text(dev ? dev.label : '—', 40, y);
                pdf.text(p.vlan || '—', 90, y);
                pdf.text(dev ? (DEVICE_TYPES[dev.type]?.label || dev.type) : '—', 115, y);
                pdf.setTextColor(100, 100, 110);
                pdf.text(dev ? (dev.notes || '—').substring(0, 50) : '—', 155, y);
                y += 5;
              });
            }

            y += 10;
          });
        }
      }

      // Footer on all pages
      const totalPages = pdf.internal.getNumberOfPages();
      for (let i = 1; i <= totalPages; i++) {
        pdf.setPage(i);
        pdf.setFontSize(8);
        pdf.setTextColor(160, 160, 170);
        pdf.text(`Plan de Plantation Réseau — Page ${i}/${totalPages}`, pageW / 2, pageH - 5, { align: 'center' });
      }

      pdf.save(`plantation-reseau-${new Date().toISOString().slice(0, 10)}.pdf`);
      showToast('PDF exporté ✓', 'success');

      selectedDeviceId = prevSelected;
      selectedConnectionId = prevConnSelected;
      renderAll();

    } catch (err) {
      console.error('PDF export error:', err);
      showToast('Erreur lors de l\'export PDF', 'error');
    }
  });

  // ── Resize handler ──
  window.addEventListener('resize', () => initViewBox());

  // ── Initialize ──
  initViewBox();
  renderAll();
  showStatus('Bienvenue ! Sélectionnez un équipement dans le panneau à gauche.');

})();
