/* =======================
   Smart Hospital Queue — Frontend
   ======================= */

let currentUser = null;
let clinicsData = [];
let stateData   = null;
let refreshTimer = null;

const API_BASE = '';

/* ---------- Low-level API ---------- */
async function api(method, endpoint, body = null) {
    const opts = { method, headers: {} };
    if (body && (method === 'POST' || method === 'PUT')) {
        opts.headers['Content-Type'] = 'application/json';
        opts.body = JSON.stringify(body);
    }
    try {
        const res = await fetch(API_BASE + endpoint, opts);
        const text = await res.text();
        try { return JSON.parse(text); }
        catch { return { status: 'error', message: 'Invalid JSON: ' + text.slice(0, 200) }; }
    } catch (err) {
        return { status: 'error', message: 'Network: ' + err.message };
    }
}

/* ---------- Login Error Helper (defined OUTSIDE doLogin) ---------- */
function setLoginError(msg) {
    const errorEl = document.getElementById('login-error');
    const nameEl  = document.getElementById('login-name');
    const phoneEl = document.getElementById('login-phone');
    
    if (errorEl) {
        errorEl.textContent = msg;
        errorEl.style.display = 'block';
    } else {
        alert(msg);
    }
    
    // Focus the relevant field
    if (msg && (msg.includes('phone') || msg.includes('mobile'))) {
        if (phoneEl) phoneEl.focus();
    } else {
        if (nameEl) nameEl.focus();
    }
}

function clearLoginError() {
    const errorEl = document.getElementById('login-error');
    if (errorEl) {
        errorEl.textContent = '';
        errorEl.style.display = 'none';
    }
}

/* ---------- Auth ---------- */
function doLogin() {
    const nameEl  = document.getElementById('login-name');
    const phoneEl = document.getElementById('login-phone');
    const name    = nameEl.value.trim();
    const phone   = phoneEl.value.trim();
    
    // Clear previous error
    clearLoginError();

    // =========================
    // Name Validation
    // =========================

    // Required
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

    // Only letters and spaces (supports English + Arabic)
    const nameRegex = /^[a-zA-Z\u0600-\u06FF\s]+$/;
    if (!nameRegex.test(name)) {
        setLoginError("Name can contain letters and spaces only.");
        return;
    }

    // =========================
    // Phone Validation
    // =========================

    // Check if staff (case-insensitive, handles "admin", "Admin", "ADMIN")
    const lowerName = name.toLowerCase();
    const isStaff = (lowerName === 'admin' || lowerName === 'receptionist');

    if (!isStaff) {
        // Required for patients
        if (!phone) {
            setLoginError("Please enter phone number.");
            return;
        }

        // Egyptian mobile validation: 01[0,1,2,5] followed by 8 digits
        const phoneRegex = /^01[0125][0-9]{8}$/;
        if (!phoneRegex.test(phone)) {
            setLoginError("Please enter a valid Egyptian mobile number.");
            return;
        }
    }

    // =========================
    // Role Assignment
    // =========================

    const role = (lowerName === 'admin') ? 'admin'
               : (lowerName === 'receptionist') ? 'receptionist'
               : 'patient';

    currentUser = { name, phone, role, patientId: null };
    sessionStorage.setItem('currentUser', JSON.stringify(currentUser));
    showScreen(role);
}

function doLogout() {
    currentUser = null;
    sessionStorage.removeItem('currentUser');
    if (refreshTimer) clearInterval(refreshTimer);
    document.querySelectorAll('.screen').forEach(s => s.classList.remove('active'));
    document.getElementById('login-screen').classList.add('active');
    clearLoginError();
}

function showScreen(role) {
    document.querySelectorAll('.screen').forEach(s => s.classList.remove('active'));
    if (role === 'admin') {
        document.getElementById('admin-screen').classList.add('active');
        initAdmin();
    } else if (role === 'receptionist') {
        document.getElementById('receptionist-screen').classList.add('active');
        initReceptionist();
    } else {
        document.getElementById('patient-screen').classList.add('active');
        document.getElementById('patient-name').textContent = currentUser.name;
        initPatient();
    }
}

/* ---------- Boot ---------- */
document.addEventListener('DOMContentLoaded', () => {
    const saved = sessionStorage.getItem('currentUser');
    if (saved) {
        try {
            currentUser = JSON.parse(saved);
            if (currentUser?.role) showScreen(currentUser.role);
        } catch { sessionStorage.removeItem('currentUser'); }
    }
});

/* ---------- Data ---------- */
async function loadClinics() {
    const res = await api('GET', '/api/clinics');
    if (res.status === 'ok' && Array.isArray(res.data)) clinicsData = res.data;
    else console.error('loadClinics failed', res);
}

async function refreshState() {
    const res = await api('GET', '/api/state');
    if (res.status === 'ok') { stateData = res.data; renderAll(); }
    else console.error('refreshState failed', res);
}

function renderAll() {
    if (!stateData) return;
    if (currentUser?.role === 'patient') renderPatientStatus();
    if (currentUser?.role === 'receptionist') {
        renderPending('pending-list', false);
        renderQueues('receptionist-queues', false);
    }
    if (currentUser?.role === 'admin') {
        renderPending('admin-pending-list', true);
        renderQueues('admin-queues', true);
        renderAdminClinics();
        renderAdminSymptoms();
    }
}

/* ---------- Patient ---------- */
function initPatient() {
    loadClinics().then(() => {
        populateClinicSelect('patient-clinic', true);
        onPatientClinicChange();
        refreshState();
        refreshTimer = setInterval(refreshState, 3000);
    });
    document.getElementById('patient-clinic').addEventListener('change', onPatientClinicChange);
    document.getElementById('patient-condition').addEventListener('change', () => {
        const box = document.getElementById('other-box');
        box.style.display = document.getElementById('patient-condition').value === 'other' ? 'block' : 'none';
    });
}

function populateClinicSelect(id, withEmpty) {
    const sel = document.getElementById(id);
    sel.innerHTML = '';
    if (withEmpty) { const o = document.createElement('option'); o.value = ''; o.textContent = '-- Select Clinic --'; sel.appendChild(o); }
    clinicsData.forEach(c => { const o = document.createElement('option'); o.value = c.id; o.textContent = `${c.name} (${c.specialty})`; sel.appendChild(o); });
}

function onPatientClinicChange() {
    const cid = parseInt(document.getElementById('patient-clinic').value);
    const sel = document.getElementById('patient-condition');
    sel.innerHTML = '';
    const clinic = clinicsData.find(c => c.id === cid);
    if (!clinic) return;
    clinic.conditions.forEach((cond, idx) => {
        const o = document.createElement('option');
        o.value = idx;
        o.textContent = `${cond.name} — ${cond.urgencyName} (~${cond.estimatedTreatmentMinutes} min)`;
        sel.appendChild(o);
    });
    const other = document.createElement('option');
    other.value = 'other';
    other.textContent = 'Other (describe symptoms)';
    sel.appendChild(other);
    sel.dispatchEvent(new Event('change'));
}

async function patientJoinQueue() {
    const clinicId = parseInt(document.getElementById('patient-clinic').value);
    const condVal  = document.getElementById('patient-condition').value;
    const symptoms = document.getElementById('patient-symptoms').value.trim();
    const msgEl    = document.getElementById('patient-message');

    if (!clinicId) { msg('Please select a clinic', 'error'); return; }

    let res;
    if (condVal === 'other') {
        if (!symptoms) { msg('Please describe your symptoms', 'error'); return; }
        res = await api('POST', '/api/add_pending_patient', {
            name: currentUser.name,
            phone: currentUser.phone,
            clinicId,
            symptomsText: symptoms
        });
    } else {
        res = await api('POST', '/api/add_patient', {
            name: currentUser.name,
            phone: currentUser.phone,
            clinicId,
            symptomIndex: parseInt(condVal)
        });
    }

    if (res.status === 'ok' && res.data) {
        msg('Successfully joined the queue!', 'success');
        if (res.data.patientId) currentUser.patientId = res.data.patientId;
        sessionStorage.setItem('currentUser', JSON.stringify(currentUser));
        refreshState();
    } else {
        msg('Error: ' + (res.message || 'Unknown'), 'error');
    }

    function msg(text, cls) { msgEl.textContent = text; msgEl.className = 'message ' + cls; }
}

function renderPatientStatus() {
    const box = document.getElementById('patient-status');
    if (!currentUser.patientId || !stateData) { box.innerHTML = '<p class="empty">Not in queue yet</p>'; return; }

    let found = null, clinicName = '', position = 0, pending = false;

    for (const c of stateData.clinics || []) {
        const q = c.waiting || [];
        for (let i = 0; i < q.length; i++) {
            if (q[i].id === currentUser.patientId) {
                found = q[i]; clinicName = c.clinicName; position = i + 1; break;
            }
        }
        if (found) break;
    }
    if (!found && stateData.pending) {
        for (const p of stateData.pending) { if (p.id === currentUser.patientId) { found = p; pending = true; break; } }
    }
    if (!found) { box.innerHTML = '<p class="empty">You are not currently in any queue.</p>'; return; }

    let html = `<div class="status-card">`;
    html += `<p><b>Clinic:</b> ${esc(clinicName) || (pending ? 'Pending Triage' : 'Unknown')}</p>`;
    html += `<p><b>Condition:</b> ${esc(found.symptom || found.reportedSymptoms || 'Other')}</p>`;
    html += `<p><b>Urgency:</b> <span class="badge urgency-${found.urgency || 0}">${esc(found.urgencyName || 'Pending')}</span></p>`;
    if (!pending) {
        html += `<p><b>Queue Position:</b> ${position}</p>`;
        html += `<p><b>Priority Score:</b> ${found.priorityScore}</p>`;
    } else {
        html += `<p><b>Status:</b> Waiting for receptionist triage</p>`;
    }
    html += `</div>`;
    box.innerHTML = html;
}

/* ---------- Receptionist / Admin shared ---------- */
function renderPending(containerId, showTriage) {
    const box = document.getElementById(containerId);
    if (!stateData?.pending?.length) { box.innerHTML = '<p class="empty">No pending patients</p>'; return; }

    const p = stateData.pending[0];
    let html = `<div class="pending-card">`;
    html += `<p><b>Next:</b> ${esc(p.name)} (ID: ${p.id})</p>`;
    html += `<p><b>Phone:</b> ${esc(p.phone || '-')}</p>`;
    html += `<p><b>Symptoms:</b> ${esc(p.reportedSymptoms || '-')}</p>`;
    html += `<p><b>Target Clinic:</b> ${esc(getClinicName(p.clinicId))}</p>`;

    if (showTriage || currentUser.role === 'receptionist') {
        html += `<div class="triage-form">`;
        html += `<label>Urgency</label><select id="triage-urgency"><option value="1">1-Elective</option><option value="2">2-Non-Urgent</option><option value="3">3-Semi-Urgent</option><option value="4">4-Urgent</option><option value="5" selected>5-Critical</option></select>`;
        html += `<label>Est. Minutes</label><input type="number" id="triage-time" value="15" min="1">`;
        html += `<button class="btn-primary" onclick="triageNext(${p.clinicId})">Triage & Route</button>`;
        html += `</div>`;
    }
    html += `</div>`;
    if (stateData.pending.length > 1) html += `<p class="subtle">+ ${stateData.pending.length - 1} more in FIFO order</p>`;
    box.innerHTML = html;
}

async function triageNext(clinicId) {
    const urgency = parseInt(document.getElementById('triage-urgency').value);
    const time    = parseInt(document.getElementById('triage-time').value);
    if (!urgency || !time) { alert('Select urgency and estimated time'); return; }
    const res = await api('POST', '/api/admin/triage_pending', { clinicId, urgencyLevel: urgency, estimatedTreatmentMinutes: time });
    if (res.status === 'ok') refreshState();
    else alert('Triage failed: ' + (res.message || 'Unknown'));
}

function renderQueues(containerId, showFinish) {
    const box = document.getElementById(containerId);
    if (!stateData?.clinics?.length) { box.innerHTML = '<p class="empty">No data</p>'; return; }

    let html = '<div class="queues-grid">';
    for (const c of stateData.clinics) {
        html += `<div class="queue-card">`;
        html += `<div class="queue-header"><h4>${esc(c.clinicName)}</h4><span class="badge">${c.queueSize} waiting</span></div>`;
        html += `<p class="subtle">${esc(c.specialty)} • Avg Wait: ${fmt(c.avgWait)} ticks • Throughput: ${fmt(c.throughput)}/hr</p>`;
        if (showFinish) {
            html += `<button class="btn-small ${c.queueSize ? 'btn-primary' : 'btn-disabled'}" onclick="finishNext(${c.clinicId})" ${!c.queueSize ? 'disabled' : ''}>Finish Next</button>`;
        }
        if (c.waiting?.length) {
            html += `<ul class="patient-list">`;
            for (const p of c.waiting) {
                html += `<li><span class="name">${esc(p.name)}</span><span class="badge urgency-${p.urgency}">${esc(p.urgencyName)}</span><span class="subtle">Pr:${p.priorityScore}</span></li>`;
            }
            html += `</ul>`;
        } else html += `<p class="empty">Queue empty</p>`;
        html += `</div>`;
    }
    html += '</div>';
    box.innerHTML = html;
}

async function finishNext(clinicId) {
    const res = await api('POST', '/api/admin/finish_next', { clinicId });
    if (res.status === 'ok') refreshState();
    else alert('Finish next failed: ' + (res.message || 'Unknown'));
}

/* ---------- Search ---------- */
async function doSearch() {
    const q = document.getElementById('search-input').value.trim();
    const box = document.getElementById('search-results');
    if (!q) { box.innerHTML = ''; return; }
    const res = await api('GET', `/api/search?q=${encodeURIComponent(q)}`);
    renderSearchResults(res, box);
}

async function doAdminSearch() {
    const q = document.getElementById('admin-search-input').value.trim();
    const box = document.getElementById('admin-search-results');
    if (!q) { box.innerHTML = ''; return; }
    const res = await api('GET', `/api/search?q=${encodeURIComponent(q)}`);
    renderSearchResults(res, box);
}

function renderSearchResults(res, container) {
    if (res.status !== 'ok' || !res.data) { container.innerHTML = `<p class="error">Search error: ${esc(res.message)}</p>`; return; }
    const r = res.data.results || [];
    if (!r.length) { container.innerHTML = '<p class="empty">No matches</p>'; return; }
    let html = `<p class="subtle">${r.length} match(es) for "${esc(res.data.query)}"</p><ul class="patient-list">`;
    for (const p of r) {
        const badgeClass = p.isPending ? 'urgency-0' : (p.urgencyName === 'Critical' ? 'urgency-5' : 'urgency-2');
        const badgeText  = p.isPending ? 'Pending' : esc(p.urgencyName);
        html += `<li><span class="name">${esc(p.name)}</span><span class="badge ${badgeClass}">${badgeText}</span><span class="subtle">ID:${p.id} Clinic:${p.clinicId}</span></li>`;
    }
    html += '</ul>';
    container.innerHTML = html;
}

/* ---------- Admin Management ---------- */
function renderAdminClinics() {
    const box = document.getElementById('admin-clinics');
    if (!clinicsData.length) { box.innerHTML = '<p class="empty">No clinics</p>'; return; }
    let html = '<ul class="manage-list">';
    for (const c of clinicsData) {
        html += `<li><span>${esc(c.name)} — ${esc(c.specialty)} (ID:${c.id})</span><button class="btn-small btn-danger" onclick="removeClinic(${c.id})">Remove</button></li>`;
    }
    html += '</ul>';
    box.innerHTML = html;

    const sel = document.getElementById('symptom-clinic-select');
    sel.innerHTML = '';
    clinicsData.forEach(c => { const o = document.createElement('option'); o.value = c.id; o.textContent = c.name; sel.appendChild(o); });
    sel.onchange = renderAdminSymptoms;
    renderAdminSymptoms();
}

function renderAdminSymptoms() {
    const cid = parseInt(document.getElementById('symptom-clinic-select').value);
    const box = document.getElementById('admin-symptoms-list');
    const c = clinicsData.find(x => x.id === cid);
    if (!c?.conditions?.length) { box.innerHTML = '<p class="empty">No symptoms for this clinic</p>'; return; }
    let html = '<ul class="manage-list">';
    for (const cond of c.conditions) {
        html += `<li><span>${esc(cond.name)} — ${esc(cond.urgencyName)} (${cond.estimatedTreatmentMinutes} min)</span><button class="btn-small btn-danger" onclick="removeSymptom(${cid}, ${cond.index})">Remove</button></li>`;
    }
    html += '</ul>';
    box.innerHTML = html;
}

async function addClinic() {
    const name = document.getElementById('new-clinic-name').value.trim();
    const spec = document.getElementById('new-clinic-spec').value.trim() || 'General';
    if (!name) { alert('Enter clinic name'); return; }
    const res = await api('POST', '/api/admin/add_clinic', { name, specialty: spec });
    if (res.status === 'ok') { document.getElementById('new-clinic-name').value = ''; document.getElementById('new-clinic-spec').value = ''; await loadClinics(); refreshState(); }
    else alert('Failed: ' + (res.message || 'Unknown'));
}

async function removeClinic(id) {
    if (!confirm('Remove this clinic?')) return;
    const res = await api('POST', '/api/admin/remove_clinic', { clinicId: id });
    if (res.status === 'ok') { await loadClinics(); refreshState(); }
    else alert('Failed: ' + (res.message || 'Unknown'));
}

async function addSymptom() {
    const clinicId = parseInt(document.getElementById('symptom-clinic-select').value);
    const name = document.getElementById('new-symptom-name').value.trim();
    const urgency = parseInt(document.getElementById('new-symptom-urgency').value);
    const time = parseInt(document.getElementById('new-symptom-time').value);
    if (!name || !urgency || !time) { alert('Fill all fields'); return; }
    const res = await api('POST', '/api/admin/add_symptom', { clinicId, name, urgencyLevel: urgency, estimatedTreatmentMinutes: time });
    if (res.status === 'ok') { document.getElementById('new-symptom-name').value = ''; document.getElementById('new-symptom-time').value = ''; await loadClinics(); renderAdminSymptoms(); }
    else alert('Failed: ' + (res.message || 'Unknown'));
}

async function removeSymptom(clinicId, idx) {
    if (!confirm('Remove this symptom?')) return;
    const res = await api('POST', '/api/admin/remove_symptom', { clinicId, symptomIndex: idx });
    if (res.status === 'ok') { await loadClinics(); renderAdminSymptoms(); }
    else alert('Failed: ' + (res.message || 'Unknown'));
}

async function doReset() {
    if (!confirm('RESET everything? This cannot be undone.')) return;
    const res = await api('POST', '/api/reset', {});
    if (res.status === 'ok') { currentUser.patientId = null; sessionStorage.setItem('currentUser', JSON.stringify(currentUser)); await loadClinics(); refreshState(); }
    else alert('Reset failed: ' + (res.message || 'Unknown'));
}

/* ---------- Utils ---------- */
function esc(t) { if (!t) return ''; const d = document.createElement('div'); d.textContent = t; return d.innerHTML; }
function fmt(n) { return (typeof n === 'number') ? (Math.round(n * 100) / 100) : '0'; }
function getClinicName(id) { const c = clinicsData.find(x => x.id === id); return c ? c.name : `Clinic ${id}`; }

function initReceptionist() {
    loadClinics().then(() => { refreshState(); refreshTimer = setInterval(refreshState, 3000); });
}

function initAdmin() {
    loadClinics().then(() => { refreshState(); refreshTimer = setInterval(refreshState, 3000); });
}