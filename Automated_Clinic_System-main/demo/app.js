const $ = (id) => document.getElementById(id);

function pretty(x) {
  try {
    return JSON.stringify(x, null, 2);
  } catch {
    return String(x);
  }
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
  showToast._t = window.setTimeout(() => {
    el.style.display = "none";
  }, 3500);
}

function setApiOut(obj) {
  const el = $("outApi");
  if (el) el.textContent = pretty(obj);
}
function setQueueOut(obj) {
  const el = $("outQueue");
  if (el) el.textContent = pretty(obj);
}

let cachedClinics = [];
let login = { name: "", phone: "", isAdmin: false };
let currentPatientId = null;
let lastState = null;

function fillClinicSelects() {
  const selects = [
    $("patientClinic"),
    $("adminClinic"),
    $("finishClinic"),
    $("viewClinic"),
    $("pendingClinic"),
  ].filter(Boolean);
  for (const sel of selects) {
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
  const clinicId = Number($("patientClinic").value || 1);
  const clinic = cachedClinics.find((c) => c.id === clinicId);
  const sel = $("patientSymptom");
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
  const clinicId = Number($("adminClinic").value || 1);
  const clinic = cachedClinics.find((c) => c.id === clinicId);
  const sel = $("adminSymptom");
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
  if (resp.status !== "ok") {
    return;
  }
  cachedClinics = resp.data || [];
  fillClinicSelects();
  fillPatientSymptoms();
  fillAdminSymptoms();
  // Default view clinic to the first option (if empty)
  const viewSel = $("viewClinic");
  if (viewSel && !viewSel.value && cachedClinics.length) {
    viewSel.value = String(cachedClinics[0].id);
  }
}

async function refreshState() {
  const resp = await api("/api/state");
  setQueueOut(resp);
  renderState(resp);
  lastState = resp && resp.status === "ok" ? resp.data : null;
  fillPendingSelect();
  if (resp && resp.status === "error") {
    showToast("error", resp.message || "Failed to load system state.");
  }
  return resp;
}

function applyMode() {
  const p = $("patientCard");
  const a = $("adminCard");
  if (p) p.style.display = login.isAdmin ? "none" : "block";
  if (a) a.style.display = login.isAdmin ? "block" : "none";
}

function setScreen(screen) {
  const loginScr = $("screenLogin");
  const appScr = $("screenApp");
  if (!loginScr || !appScr) return;
  if (screen === "login") {
    loginScr.style.display = "grid";
    appScr.style.display = "none";
  } else {
    loginScr.style.display = "none";
    appScr.style.display = "block";
  }
}

function setLoginError(msg) {
  const el = $("loginError");
  if (!el) return;
  if (!msg) {
    el.style.display = "none";
    el.textContent = "";
  } else {
    el.style.display = "block";
    el.textContent = msg;
  }
}

function updateWhoAmI() {
  const el = $("whoami");
  if (!el) return;
  if (!login.name) {
    el.textContent = "Not signed in";
  } else if (login.isAdmin) {
    el.textContent = "Admin";
  } else {
    el.textContent = `${login.name} • ${login.phone || "No phone"}`;
  }
}

function badgeClassByUrgencyName(name) {
  const n = String(name || "").toLowerCase();
  if (n.includes("critical")) return "badge badgeCritical";
  if (n.includes("urgent")) return "badge badgeUrgent";
  return "badge badgeNormal";
}

function renderState(resp) {
  if (!resp || resp.status !== "ok") return;
  const data = resp.data || {};
  const clinics = data.clinics || [];

  const viewSel = $("viewClinic");
  const viewClinicId = viewSel ? Number(viewSel.value || (clinics[0]?.clinicId ?? clinics[0]?.id ?? 1)) : (clinics[0]?.clinicId ?? clinics[0]?.id ?? 1);
  const clinic = clinics.find((c) => Number(c.clinicId || c.id) === Number(viewClinicId)) || clinics[0];

  const title = $("queueTitle");
  const meta = $("queueMeta");
  const list = $("queueList");
  if (!clinic || !title || !meta || !list) return;

  title.textContent = clinic.clinicName || clinic.name || "Clinic";
  meta.textContent = `${clinic.queueSize ?? (clinic.waiting?.length ?? 0)} waiting`;

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

  // Patient status (if logged in as patient)
  const statusEl = $("patientStatus");
  const posEl = $("patientPosition");
  if (statusEl && posEl && !login.isAdmin) {
    if (!currentPatientId) {
      statusEl.textContent = "Not registered yet";
      posEl.textContent = "—";
    } else {
      let found = null;
      let foundClinic = null;
      let position = null;
      for (const c of clinics) {
        const w = c.waiting || [];
        for (let i = 0; i < w.length; i++) {
          if (Number(w[i].id) === Number(currentPatientId)) {
            found = w[i];
            foundClinic = c;
            position = i + 1;
            break;
          }
        }
        if (found) break;
      }
      if (found) {
        statusEl.textContent = `Waiting in ${foundClinic.clinicName || foundClinic.name}`;
        posEl.textContent = `#${position}`;
      } else {
        statusEl.textContent = "Finished / not in queue";
        posEl.textContent = "—";
      }
    }
  }
}

function fillPendingSelect() {
  const sel = $("pendingSelect");
  const details = $("pendingDetails");
  if (!sel || !details) return;
  sel.innerHTML = "";
  const pending = (lastState && lastState.pending) ? lastState.pending : [];
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
  details.textContent = `Symptoms: ${first.reportedSymptoms || "—"}`;
}

async function init() {
  $("btnReset").addEventListener("click", async () => {
    const resp = await api("/api/reset", "POST", {});
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "System reset.");
    else showToast("error", resp.message || "Reset failed.");
    await refreshState();
  });

  $("btnLogin").addEventListener("click", async () => {
    const name = $("loginName").value.trim();
    const phone = $("loginPhone").value.trim();
    setLoginError("");
      /*Validate Area */
      if (!name) {
          setLoginError("Please enter your name.");
          return;
      }
      // Min length
      if (name.length < 3) {
          setLoginError("Name must be at least 3 characters.");
          return;
      }
      // Max length
      if (name.length > 30) {
          setLoginError("Name is too long.");
          return;
      }
      // Only letters and spaces
      const nameRegex = /^[a-zA-Z\u0600-\u06FF\s]+$/;
      if (!nameRegex.test(name)) {
          setLoginError("Name can contain letters only.");
          return;
      }
      // =========================
      // Phone Validation
      // =========================
      // Required
      if (!phone) {
          setLoginError("Please enter phone number.");
          return;
      }
      // Egyptian mobile validation
      // يبدأ بـ 010 / 011 / 012 / 015 + 8 digits
      const phoneRegex = /^01[0125][0-9]{8}$/;
      if (!phoneRegex.test(phone)) {
          setLoginError("Please enter a valid Egyptian mobile number.");
          return;
      }
    login = { name, phone, isAdmin: name === "Admin" && phone === "" };
    currentPatientId = null;
    setScreen("app");
    applyMode();
    updateWhoAmI();
    setApiOut({ status: "ok", data: { login } });
    await loadClinics();
    await refreshState();
  });

  $("patientClinic").addEventListener("change", () => {
    fillPatientSymptoms();
  });

  $("patientSymptom").addEventListener("change", () => {
    const v = $("patientSymptom").value;
    $("otherSymptomWrap").style.display = v === "other" ? "block" : "none";
  });

  $("adminClinic").addEventListener("change", () => {
    fillAdminSymptoms();
  });

  $("btnRegister").addEventListener("click", async () => {
    if (!login.name) {
      setApiOut({ status: "error", message: "Login first." });
      return;
    }
    const clinicId = Number($("patientClinic").value);
    const symptomVal = $("patientSymptom").value;
    let resp;
    if (symptomVal === "other") {
      const symptomsText = $("otherSymptomText").value.trim();
      resp = await api("/api/add_pending_patient", "POST", {
        name: login.name,
        phone: login.phone,
        clinicId,
        symptomsText,
      });
      if (resp.status === "ok") showToast("ok", "Submitted for triage.");
      else showToast("error", resp.message || "Could not submit.");
    } else {
      const symptomIndex = Number(symptomVal);
      resp = await api("/api/add_patient", "POST", {
        name: login.name,
        phone: login.phone,
        clinicId,
        symptomIndex,
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

  $("btnRefresh").addEventListener("click", async () => {
    await loadClinics();
    await refreshState();
  });

  $("btnAddClinic").addEventListener("click", async () => {
    const name = $("newClinicName").value.trim();
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
    const name = $("newSymptomName").value.trim();
    const urgencyLevel = Number($("newSymptomUrgency").value);
    const estimatedTreatmentMinutes = Number($("newSymptomEst").value);
    const resp = await api("/api/admin/add_symptom", "POST", {
      clinicId,
      name,
      urgencyLevel,
      estimatedTreatmentMinutes,
    });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "Symptom added.");
    else showToast("error", resp.message || "Failed to add symptom.");
    await loadClinics();
    await refreshState();
  });

  $("btnRemoveSymptom").addEventListener("click", async () => {
    const clinicId = Number($("adminClinic").value);
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

  $("pendingSelect").addEventListener("change", () => {
    const pending = (lastState && lastState.pending) ? lastState.pending : [];
    const id = Number($("pendingSelect").value);
    const p = pending.find((x) => Number(x.id) === id) || pending[0];
    $("pendingDetails").textContent = p ? `Symptoms: ${p.reportedSymptoms || "—"}` : "";
  });

  $("btnTriagePending").addEventListener("click", async () => {
    const clinicId = Number($("pendingClinic").value);
    const urgencyLevel = Number($("pendingUrgency").value);
    const estimatedTreatmentMinutes = Number($("pendingEst").value);
    const resp = await api("/api/admin/triage_pending", "POST", {
      clinicId,
      urgencyLevel,
      estimatedTreatmentMinutes,
    });
    setApiOut(resp);
    if (resp.status === "ok") showToast("ok", "Patient triaged and queued.");
    else showToast("error", resp.message || "Triage failed.");
    await refreshState();
  });

  $("viewClinic").addEventListener("change", async () => {
    await refreshState();
  });

  $("btnLogout").addEventListener("click", () => {
    login = { name: "", phone: "", isAdmin: false };
    currentPatientId = null;
    $("loginName").value = "";
    $("loginPhone").value = "";
    setLoginError("");
    updateWhoAmI();
    setScreen("login");
  });

  // Start on login screen
  setScreen("login");
  applyMode();
  updateWhoAmI();
}

init().catch((e) => {
  setApiOut({ status: "error", message: String(e) });
});

