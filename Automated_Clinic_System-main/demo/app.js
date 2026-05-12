// ============================================================
// Smart Hospital Demo — app.js
// Supports four roles: Patient, Doctor, Receptionist, Admin
// ============================================================

const $ = (id) => document.getElementById(id);

function pretty(x) {
  try { return JSON.stringify(x, null, 2); } catch { return String(x); }
}

async function api(path, method = "GET", body = null) {
  const init = { method, headers: {} };
  if (body !== null) {
    init.headers["Content-Type"] = "application/json";
    init.body = JSON.stringify(body);
  }
  const res = await fetch(path, init);
  return await res.json();
}

function showToast(kind, message) {
  const el = $("toast");
  if (!el) return;
  el.className = `toast ${kind === "error" ? "toastErr" : "toastOk"}`;
  el.textContent = message;
  el.style.display = "block";
  window.clearTimeout(showToast._t);
  showToast._t = window.setTimeout(() => { el.style.display = "none"; }, 3500);
}

function setApiOut(obj)   { const el = $("outApi");   if (el) el.textContent = pretty(obj); }
function setQueueOut(obj) { const el = $("outQueue"); if (el) el.textContent = pretty(obj); }

// ============================================================
// State
// ============================================================
let cachedClinics = [];
let lastState     = null;
let currentPatientId = null;

// login object: { name, phone, role }
// role: "patient" | "doctor" | "receptionist" | "admin"
let login = { name: "", phone: "", role: "" };

// Staff PINs (hardcoded for demo purposes)
const STAFF_PINS = {
  doctor:       "doc123",
  receptionist: "rec123",
  admin:        "admin",
};

// ============================================================
// Utility: show/hide PIN input based on role
// ============================================================
function updateLoginForm() {
  const role = $("loginRole").value;
  const pinWrap = $("staffPinWrap");
  const phoneLabel = $("loginPhone").parentElement;
  if (role === "doctor" || role === "receptionist" || role === "admin") {
    pinWrap.style.display = "block";
    // Phone is not required for staff; hide validation visually but keep the field
  } else {
    pinWrap.style.display = "none";
  }
}

// ============================================================
// Role-aware view switching
// ============================================================
function applyMode() {
  const role = login.role;
  // Cards
  const patientCard      = $("patientCard");
  const doctorCard       = $("doctorCard");
  const receptionistCard = $("receptionistCard");
  const adminCard        = $("adminCard");

  if (patientCard)      patientCard.style.display      = role === "patient"      ? "block" : "none";
  if (doctorCard)       doctorCard.style.display        = role === "doctor"       ? "block" : "none";
  if (receptionistCard) receptionistCard.style.display  = role === "receptionist" ? "block" : "none";
  if (adminCard)        adminCard.style.display          = role === "admin"        ? "block" : "none";
}

function setScreen(screen) {
  const loginScr = $("screenLogin");
  const appScr   = $("screenApp");
  if (!loginScr || !appScr) return;
  if (screen === "login") {
    loginScr.style.display = "grid";
    appScr.style.display   = "none";
  } else {
    loginScr.style.display = "none";
    appScr.style.display   = "block";
  }
}

function setLoginError(msg) {
  const el = $("loginError");
  if (!el) return;
  if (!msg) { el.style.display = "none"; el.textContent = ""; }
  else       { el.style.display = "block"; el.textContent = msg; }
}

function updateWhoAmI() {
  const el = $("whoami");
  if (!el) return;
  if (!login.name) { el.textContent = "Not signed in"; return; }

  const roleLabel = {
    patient:      "Patient",
    doctor:       "Doctor",
    receptionist: "Receptionist",
    admin:        "Admin",
  }[login.role] || login.role;

  const badgeClass = {
    patient:      "roleBadgePatient",
    doctor:       "roleBadgeDoctor",
    receptionist: "roleBadgeReceptionist",
    admin:        "roleBadgeAdmin",
  }[login.role] || "";

  const phonePart = login.phone ? ` • ${login.phone}` : "";
  el.innerHTML = `${login.name}${phonePart} <span class="roleBadge ${badgeClass}">${roleLabel}</span>`;
}

// ============================================================
// Clinic selects
// ============================================================
function fillClinicSelects() {
  const ids = [
    "patientClinic", "adminClinic", "finishClinic",
    "viewClinic", "pendingClinic", "adminPendingClinic",
  ];
  for (const id of ids) {
    const sel = $(id);
    if (!sel) continue;
    sel.innerHTML = "";
    for (const c of cachedClinics) {
      const opt = document.createElement("option");
      opt.value = String(c.id);
      opt.textContent = `${c.id} - ${c.name}`;
      sel.appendChild(opt);
    }
  }
}

function fillPatientSymptoms() {
  const clinicId = Number(($("patientClinic") || {}).value || 1);
  const clinic = cachedClinics.find((c) => c.id === clinicId);
  const sel = $("patientSymptom");
  if (!sel) return;
  sel.innerHTML = "";
  if (!clinic) return;
  for (const cond of clinic.conditions || []) {
    const opt = document.createElement("option");
    opt.value = String(cond.index);
    opt.textContent = `${cond.name} (${cond.urgencyName}, ~${cond.estimatedTreatmentMinutes}m)`;
    sel.appendChild(opt);
  }
  const other = document.createElement("option");
  other.value = "other";
  other.textContent = "Other (write symptoms)";
  sel.appendChild(other);
}

function fillAdminSymptoms() {
  const clinicId = Number(($("adminClinic") || {}).value || 1);
  const clinic = cachedClinics.find((c) => c.id === clinicId);
  const sel = $("adminSymptom");
  if (!sel) return;
  sel.innerHTML = "";
  if (!clinic) return;
  for (const cond of clinic.conditions || []) {
    const opt = document.createElement("option");
    opt.value = String(cond.index);
    opt.textContent = `${cond.index} - ${cond.name} (${cond.urgencyName})`;
    sel.appendChild(opt);
  }
}

async function loadClinics() {
  const resp = await api("/api/clinics");
  setApiOut(resp);
  if (resp.status !== "ok") return;
  cachedClinics = resp.data || [];
  fillClinicSelects();
  fillPatientSymptoms();
  fillAdminSymptoms();
  const viewSel = $("viewClinic");
  if (viewSel && !viewSel.value && cachedClinics.length) {
    viewSel.value = String(cachedClinics[0].id);
  }
}

// ============================================================
// State refresh
// ============================================================
async function refreshState() {
  const resp = await api("/api/state");
  setQueueOut(resp);
  renderQueuePanel(resp);
  renderDoctorPanel(resp);
  lastState = (resp && resp.status === "ok") ? resp.data : null;
  fillPendingSelect("pendingSelect", "pendingDetails", "pendingCount");
  fillPendingSelect("adminPendingSelect", "adminPendingDetails", null);
  if (resp && resp.status === "error") {
    showToast("error", resp.message || "Failed to load system state.");
  }
  return resp;
}

// ============================================================
// Queue panel renderer
// ============================================================
function badgeClassByUrgencyName(name) {
  const n = String(name || "").toLowerCase();
  if (n.includes("critical")) return "badge badgeCritical";
  if (n.includes("urgent"))   return "badge badgeUrgent";
  return "badge badgeNormal";
}

function renderQueuePanel(resp) {
  if (!resp || resp.status !== "ok") return;
  const data    = resp.data || {};
  const clinics = data.clinics || [];

  const viewSel     = $("viewClinic");
  const viewClinicId = viewSel
    ? Number(viewSel.value || (clinics[0]?.clinicId ?? clinics[0]?.id ?? 1))
    : (clinics[0]?.clinicId ?? clinics[0]?.id ?? 1);
  const clinic = clinics.find((c) => Number(c.clinicId || c.id) === Number(viewClinicId)) || clinics[0];

  const title = $("queueTitle");
  const meta  = $("queueMeta");
  const list  = $("queueList");
  if (!clinic || !title || !meta || !list) return;

  title.textContent = clinic.clinicName || clinic.name || "Clinic";
  meta.textContent  = `${clinic.queueSize ?? (clinic.waiting?.length ?? 0)} waiting`;

  const waiting = clinic.waiting || [];
  list.innerHTML = "";
  if (waiting.length === 0) {
    list.innerHTML = `<div class="queueRow"><div class="queueCellMuted" style="grid-column:1/-1">No patients waiting.</div></div>`;
  } else {
    waiting.forEach((p, idx) => {
      const row = document.createElement("div");
      row.className = "queueRow";
      const urgencyName = p.urgencyName || p.urgency || "";
      row.innerHTML = `
        <div class="queueCellMuted">#${idx + 1}</div>
        <div>
          <div style="font-weight:650">${p.name || "Unnamed"}</div>
          <div class="queueCellMuted">${p.phone ? p.phone : "No phone"}</div>
        </div>
        <div>
          <div>${p.symptom || "—"}</div>
          <div class="queueCellMuted">Priority: ${p.priorityScore ?? "—"}</div>
        </div>
        <div><span class="${badgeClassByUrgencyName(urgencyName)}">${urgencyName || "Normal"}</span></div>
      `;
      list.appendChild(row);
    });
  }

  // Patient status tracker
  const statusEl = $("patientStatus");
  const posEl    = $("patientPosition");
  if (statusEl && posEl && login.role === "patient") {
    if (!currentPatientId) {
      statusEl.textContent = "Not registered yet";
      posEl.textContent    = "—";
    } else {
      let found = null; let foundClinic = null; let position = null;
      for (const c of clinics) {
        const w = c.waiting || [];
        for (let i = 0; i < w.length; i++) {
          if (Number(w[i].id) === Number(currentPatientId)) {
            found = w[i]; foundClinic = c; position = i + 1; break;
          }
        }
        if (found) break;
      }
      if (found) {
        statusEl.textContent = `Waiting in ${foundClinic.clinicName || foundClinic.name}`;
        posEl.textContent    = `#${position}`;
      } else {
        statusEl.textContent = "Finished / not in queue";
        posEl.textContent    = "—";
      }
    }
  }
}

// ============================================================
// Doctor panel renderer
// ============================================================
function renderDoctorPanel(resp) {
  const container = $("doctorClinicList");
  if (!container) return;
  if (!resp || resp.status !== "ok") return;

  const clinics = (resp.data || {}).clinics || [];
  container.innerHTML = "";

  if (clinics.length === 0) {
    container.innerHTML = `<div class="muted small">No clinics available.</div>`;
    return;
  }

  for (const c of clinics) {
    const clinicId   = c.clinicId || c.id;
    const clinicName = c.clinicName || c.name || `Clinic ${clinicId}`;
    const waiting    = c.waiting || [];
    const queueSize  = c.queueSize ?? waiting.length;
    const nextPat    = waiting[0];

    const card = document.createElement("div");
    card.className = "doctorClinicCard";

    const nextInfo = nextPat
      ? `Next: <strong>${nextPat.name}</strong> — ${nextPat.symptom || "—"} <span class="${badgeClassByUrgencyName(nextPat.urgencyName)}">${nextPat.urgencyName || "Normal"}</span>`
      : "Queue is empty";

    card.innerHTML = `
      <div class="doctorClinicTitle">${clinicName}</div>
      <div class="doctorQueueSize">${queueSize} patient${queueSize !== 1 ? "s" : ""} waiting</div>
      <div class="doctorNextPatient">${nextInfo}</div>
      <button class="btn btnDoctor" data-clinic="${clinicId}" ${queueSize === 0 ? "disabled" : ""}>
        ✅ Finish Patient
      </button>
    `;
    container.appendChild(card);
  }

  // Attach finish handlers
  container.querySelectorAll("button[data-clinic]").forEach((btn) => {
    btn.addEventListener("click", async () => {
      const clinicId = Number(btn.dataset.clinic);
      btn.disabled = true;
      const resp = await api("/api/admin/finish_next", "POST", { clinicId });
      setApiOut(resp);
      if (resp.status === "ok") {
        showToast("ok", `Finished patient #${resp.data?.finishedPatientId ?? ""}`.trim());
      } else {
        showToast("error", resp.message || "Finish failed.");
      }
      await refreshState();
    });
  });
}

// ============================================================
// Pending patient select (Receptionist + Admin shared)
// ============================================================
function fillPendingSelect(selectId, detailsId, countId) {
  const sel     = $(selectId);
  const details = $(detailsId);
  if (!sel || !details) return;

  sel.innerHTML = "";
  const pending = (lastState && lastState.pending) ? lastState.pending : [];

  if (countId) {
    const countEl = $(countId);
    if (countEl) {
      countEl.textContent = pending.length
        ? `${pending.length} patient${pending.length !== 1 ? "s" : ""} awaiting triage`
        : "No pending patients";
    }
  }

  if (!pending.length) {
    const opt = document.createElement("option");
    opt.value = "";
    opt.textContent = "No pending patients";
    sel.appendChild(opt);
    details.textContent = "";
    return;
  }
  for (const p of pending) {
    const opt = document.createElement("option");
    opt.value = String(p.id);
    opt.textContent = `#${p.id} - ${p.name}`;
    sel.appendChild(opt);
  }
  const first = pending[0];
  details.innerHTML = first
    ? `<strong>${first.name}</strong> — Symptoms: ${first.reportedSymptoms || "—"}`
    : "";
}

// ============================================================
// Login handler
// ============================================================
async function handleLogin() {
  const name  = $("loginName").value.trim();
  const phone = $("loginPhone").value.trim();
  const role  = $("loginRole").value;
  const pin   = ($("loginPin") || {}).value || "";

  setLoginError("");

  // Name validation for all roles
  if (!name) { setLoginError("Please enter your name."); return; }
  if (name.length < 3) { setLoginError("Name must be at least 3 characters."); return; }
  if (name.length > 30) { setLoginError("Name is too long."); return; }
  const nameRegex = /^[a-zA-Z\u0600-\u06FF\s]+$/;
  if (!nameRegex.test(name)) { setLoginError("Name can contain letters only."); return; }

  if (role === "patient") {
    // Phone required for patients
    if (!phone) { setLoginError("Please enter phone number."); return; }
    const phoneRegex = /^01[0125][0-9]{8}$/;
    if (!phoneRegex.test(phone)) { setLoginError("Please enter a valid Egyptian mobile number."); return; }
  } else {
    // Staff: validate PIN
    const expectedPin = STAFF_PINS[role];
    if (!pin) { setLoginError("Please enter staff PIN."); return; }
    if (pin !== expectedPin) { setLoginError("Incorrect staff PIN."); return; }
  }

  login = { name, phone: role === "patient" ? phone : "", role };
  currentPatientId = null;
  setScreen("app");
  applyMode();
  updateWhoAmI();
  setApiOut({ status: "ok", data: { login } });
  await loadClinics();
  await refreshState();
}

// ============================================================
// init
// ============================================================
async function init() {
  // Login role change → toggle PIN visibility
  $("loginRole").addEventListener("change", updateLoginForm);
  updateLoginForm();

  $("btnLogin").addEventListener("click", handleLogin);

  $("loginName").addEventListener("keydown", (e) => { if (e.key === "Enter") handleLogin(); });
  $("loginPhone").addEventListener("keydown", (e) => { if (e.key === "Enter") handleLogin(); });
  ($("loginPin") || {}).addEventListener && $("loginPin").addEventListener("keydown", (e) => {
    if (e.key === "Enter") handleLogin();
  });

  // ---- Logout ----
  $("btnLogout").addEventListener("click", () => {
    login = { name: "", phone: "", role: "" };
    currentPatientId = null;
    $("loginName").value  = "";
    $("loginPhone").value = "";
    if ($("loginPin")) $("loginPin").value = "";
    setLoginError("");
    updateWhoAmI();
    setScreen("login");
  });

  // ---- Reset / Refresh ----
  $("btnReset").addEventListener("click", async () => {
    const resp = await api("/api/reset", "POST", {});
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "System reset.");
    else showToast("error", resp.message || "Reset failed.");
    await loadClinics();
    await refreshState();
  });

  $("btnRefresh").addEventListener("click", async () => {
    await loadClinics();
    await refreshState();
  });

  // ---- Patient: clinic / symptom ----
  $("patientClinic").addEventListener("change", () => fillPatientSymptoms());
  $("patientSymptom").addEventListener("change", () => {
    const v = $("patientSymptom").value;
    $("otherSymptomWrap").style.display = v === "other" ? "block" : "none";
  });

  $("btnRegister").addEventListener("click", async () => {
    if (!login.name) { setApiOut({ status: "error", message: "Login first." }); return; }
    const clinicId   = Number($("patientClinic").value);
    const symptomVal = $("patientSymptom").value;
    let resp;
    if (symptomVal === "other") {
      const symptomsText = $("otherSymptomText").value.trim();
      resp = await api("/api/add_pending_patient", "POST", {
        name: login.name, phone: login.phone, clinicId, symptomsText,
      });
      if (resp.status === "ok") showToast("ok", "Submitted for triage.");
      else showToast("error", resp.message || "Could not submit.");
    } else {
      resp = await api("/api/add_patient", "POST", {
        name: login.name, phone: login.phone, clinicId,
        symptomIndex: Number(symptomVal),
      });
      if (resp.status === "ok") showToast("ok", "You joined the queue.");
      else showToast("error", resp.message || "Could not join queue.");
    }
    setApiOut(resp);
    if (resp && resp.status === "ok" && resp.data && resp.data.patientId) {
      currentPatientId = resp.data.patientId;
      $("patientStatus").textContent = "Registered. Loading queue…";
    }
    await refreshState();
  });

  // ---- Queue viewer ----
  $("viewClinic").addEventListener("change", async () => { await refreshState(); });

  // ---- Admin: clinic management ----
  $("adminClinic").addEventListener("change", () => fillAdminSymptoms());

  $("btnAddClinic").addEventListener("click", async () => {
    const name      = $("newClinicName").value.trim();
    const specialty = $("newClinicSpec").value.trim();
    const resp = await api("/api/admin/add_clinic", "POST", { name, specialty });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "Clinic added.");
    else showToast("error", resp.message || "Failed to add clinic.");
    await loadClinics();
    await refreshState();
  });

  $("btnRemoveClinic").addEventListener("click", async () => {
    const clinicId = Number($("adminClinic").value);
    const resp = await api("/api/admin/remove_clinic", "POST", { clinicId });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "Clinic removed.");
    else showToast("error", resp.message || "Failed to remove clinic.");
    await loadClinics();
    await refreshState();
  });

  $("btnAddSymptom").addEventListener("click", async () => {
    const clinicId = Number($("adminClinic").value);
    const name     = $("newSymptomName").value.trim();
    const urgencyLevel = Number($("newSymptomUrgency").value);
    const estimatedTreatmentMinutes = Number($("newSymptomEst").value);
    const resp = await api("/api/admin/add_symptom", "POST", {
      clinicId, name, urgencyLevel, estimatedTreatmentMinutes,
    });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "Symptom added.");
    else showToast("error", resp.message || "Failed to add symptom.");
    await loadClinics();
    await refreshState();
  });

  $("btnRemoveSymptom").addEventListener("click", async () => {
    const clinicId    = Number($("adminClinic").value);
    const symptomIndex = Number($("adminSymptom").value);
    const resp = await api("/api/admin/remove_symptom", "POST", { clinicId, symptomIndex });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "Symptom removed.");
    else showToast("error", resp.message || "Failed to remove symptom.");
    await loadClinics();
    await refreshState();
  });

  $("btnFinishNext").addEventListener("click", async () => {
    const clinicId = Number($("finishClinic").value);
    const resp = await api("/api/admin/finish_next", "POST", { clinicId });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", `Finished patient #${resp.data?.finishedPatientId ?? ""}`.trim());
    else showToast("error", resp.message || "Finish failed.");
    await refreshState();
  });

  // ---- Admin triage panel ----
  $("adminPendingSelect").addEventListener("change", () => {
    const pending = (lastState && lastState.pending) ? lastState.pending : [];
    const id = Number($("adminPendingSelect").value);
    const p  = pending.find((x) => Number(x.id) === id) || pending[0];
    $("adminPendingDetails").innerHTML = p
      ? `<strong>${p.name}</strong> — Symptoms: ${p.reportedSymptoms || "—"}`
      : "";
  });

  $("btnAdminTriagePending").addEventListener("click", async () => {
    const clinicId  = Number($("adminPendingClinic").value);
    const urgencyLevel = Number($("adminPendingUrgency").value);
    const estimatedTreatmentMinutes = Number($("adminPendingEst").value);
    const resp = await api("/api/admin/triage_pending", "POST", {
      clinicId, urgencyLevel, estimatedTreatmentMinutes,
    });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "Patient triaged and queued.");
    else showToast("error", resp.message || "Triage failed.");
    await refreshState();
  });

  // ============================================================
  // RECEPTIONIST — pending select + triage + save diagnosis
  // ============================================================
  $("pendingSelect").addEventListener("change", () => {
    const pending = (lastState && lastState.pending) ? lastState.pending : [];
    const id = Number($("pendingSelect").value);
    const p  = pending.find((x) => Number(x.id) === id) || pending[0];
    $("pendingDetails").innerHTML = p
      ? `<strong>${p.name}</strong> — Symptoms: ${p.reportedSymptoms || "—"}`
      : "";
  });

  // Toggle "save diagnosis name" input visibility
  $("pendingSave").addEventListener("change", () => {
    $("pendingSaveNameWrap").style.display = $("pendingSave").checked ? "block" : "none";
    if ($("pendingSave").checked) {
      // Pre-fill the label with the selected patient's reported symptoms as a suggestion
      const pending = (lastState && lastState.pending) ? lastState.pending : [];
      const id = Number($("pendingSelect").value);
      const p  = pending.find((x) => Number(x.id) === id) || pending[0];
      if (p && p.reportedSymptoms) {
        const nameInput = $("pendingSaveName");
        if (nameInput && !nameInput.value) nameInput.value = p.reportedSymptoms.slice(0, 40);
      }
    }
  });

  $("btnTriagePending").addEventListener("click", async () => {
    const clinicId  = Number($("pendingClinic").value);
    const urgencyLevel = Number($("pendingUrgency").value);
    const estimatedTreatmentMinutes = Number($("pendingEst").value);
    const shouldSave = $("pendingSave").checked;
    const saveName   = ($("pendingSaveName").value || "").trim();

    if (estimatedTreatmentMinutes <= 0) {
      showToast("error", "Estimated minutes must be greater than 0.");
      return;
    }

    // 1. Triage the patient
    const triageResp = await api("/api/admin/triage_pending", "POST", {
      clinicId, urgencyLevel, estimatedTreatmentMinutes,
    });
    setApiOut(triageResp);

    if (triageResp.status !== "ok") {
      showToast("error", triageResp.message || "Triage failed.");
      await refreshState();
      return;
    }

    showToast("ok", `Patient triaged and routed to clinic.`);

    // 2. Optionally save diagnosis as a new common condition
    if (shouldSave && saveName) {
      const saveResp = await api("/api/admin/add_symptom", "POST", {
        clinicId,
        name: saveName,
        urgencyLevel,
        estimatedTreatmentMinutes,
      });
      if (saveResp.status === "ok") {
        showToast("ok", `Diagnosis "${saveName}" saved for future use ✅`);
      } else {
        showToast("error", `Triage done, but could not save diagnosis: ${saveResp.message || "unknown error"}`);
      }
    }

    // Reset checkbox + name field
    $("pendingSave").checked = false;
    $("pendingSaveName").value = "";
    $("pendingSaveNameWrap").style.display = "none";

    await loadClinics(); // refresh symptom list if a new one was saved
    await refreshState();
  });

  // ============================================================
  // Start on login screen
  // ============================================================
  setScreen("login");
  applyMode();
  updateWhoAmI();
}

init().catch((e) => { setApiOut({ status: "error", message: String(e) }); });
