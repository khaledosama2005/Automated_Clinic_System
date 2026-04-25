const URGENCY = {
  1: { name: "Elective", className: "badge-elective" },
  2: { name: "Non-Urgent", className: "badge-non" },
  3: { name: "Semi-Urgent", className: "badge-semi" },
  4: { name: "Urgent", className: "badge-urgent" },
  5: { name: "Critical", className: "badge-critical" }
};

const clinics = [
  {
    id: 1,
    name: "Emergency",
    specialty: "Emergency Medicine",
    conditions: [
      { name: "Chest Pain", urgency: 5, estimatedTreatmentMinutes: 45 },
      { name: "Severe Bleeding", urgency: 5, estimatedTreatmentMinutes: 40 },
      { name: "High Fever", urgency: 4, estimatedTreatmentMinutes: 25 },
      { name: "Minor Cut", urgency: 2, estimatedTreatmentMinutes: 10 }
    ]
  },
  {
    id: 2,
    name: "Cardiology",
    specialty: "Heart & Blood Pressure",
    conditions: [
      { name: "Irregular Heartbeat", urgency: 4, estimatedTreatmentMinutes: 35 },
      { name: "High Blood Pressure", urgency: 3, estimatedTreatmentMinutes: 25 },
      { name: "Routine Follow-up", urgency: 1, estimatedTreatmentMinutes: 15 }
    ]
  },
  {
    id: 3,
    name: "Orthopedics",
    specialty: "Bones & Joints",
    conditions: [
      { name: "Broken Bone", urgency: 4, estimatedTreatmentMinutes: 40 },
      { name: "Sprain", urgency: 2, estimatedTreatmentMinutes: 20 },
      { name: "Back Pain", urgency: 2, estimatedTreatmentMinutes: 25 },
      { name: "Routine Cast Check", urgency: 1, estimatedTreatmentMinutes: 15 }
    ]
  }
];

let patients = JSON.parse(localStorage.getItem("patients") || "[]");
let nextPatientId = Number(localStorage.getItem("nextPatientId") || "1");

const clinicSelect = document.getElementById("clinicSelect");
const conditionSelect = document.getElementById("conditionSelect");
const otherConditionBox = document.getElementById("otherConditionBox");
const patientForm = document.getElementById("patientForm");
const queuesContainer = document.getElementById("queuesContainer");
const refreshBtn = document.getElementById("refreshBtn");

function saveState() {
  localStorage.setItem("patients", JSON.stringify(patients));
  localStorage.setItem("nextPatientId", String(nextPatientId));
}

function getCurrentTick() {
  return Math.floor(Date.now() / 1000);
}

function computePriority(patient) {
  const agingCounter = Math.max(0, getCurrentTick() - patient.arrivalTime);
  return patient.urgency * 1000000 - patient.arrivalTime + agingCounter * 100;
}

function populateClinics() {
  clinicSelect.innerHTML = clinics
    .map((clinic) => `<option value="${clinic.id}">${clinic.name}</option>`)
    .join("");
}

function populateConditions() {
  const clinic = clinics.find((item) => item.id === Number(clinicSelect.value));
  const options = clinic.conditions
    .map((condition, index) => `<option value="${index}">${condition.name} — ${URGENCY[condition.urgency].name}</option>`)
    .join("");

  conditionSelect.innerHTML = options + `<option value="other">Other</option>`;
  toggleOtherFields();
}

function toggleOtherFields() {
  otherConditionBox.classList.toggle("hidden", conditionSelect.value !== "other");
}

function addPatient(event) {
  event.preventDefault();

  const clinicId = Number(clinicSelect.value);
  const clinic = clinics.find((item) => item.id === clinicId);
  const selectedCondition = conditionSelect.value;
  let condition;

  if (selectedCondition === "other") {
    condition = {
      name: document.getElementById("otherConditionName").value.trim() || "Other",
      urgency: Number(document.getElementById("otherUrgencySelect").value),
      estimatedTreatmentMinutes: Number(document.getElementById("otherTreatmentTime").value || 20)
    };
  } else {
    condition = clinic.conditions[Number(selectedCondition)];
  }

  const patient = {
    id: nextPatientId++,
    name: document.getElementById("patientName").value.trim(),
    clinicId,
    clinicName: clinic.name,
    conditionName: condition.name,
    urgency: condition.urgency,
    estimatedTreatmentMinutes: condition.estimatedTreatmentMinutes,
    arrivalTime: getCurrentTick()
  };

  patients.push(patient);
  saveState();
  patientForm.reset();
  populateConditions();
  renderQueues();
}

function getSortedPatientsForClinic(clinicId) {
  return patients
    .filter((patient) => patient.clinicId === clinicId)
    .map((patient) => ({ ...patient, priorityScore: computePriority(patient) }))
    .sort((a, b) => {
      if (b.priorityScore !== a.priorityScore) return b.priorityScore - a.priorityScore;
      return a.arrivalTime - b.arrivalTime;
    });
}

function renderPatient(patient, rank) {
  const urgency = URGENCY[patient.urgency];
  const article = document.createElement("article");
  article.className = "patient-card";
  article.innerHTML = `
    <div class="patient-main">
      <strong class="patient-name">#${rank} — ${patient.name}</strong>
      <span class="patient-condition">${patient.conditionName} · ~${patient.estimatedTreatmentMinutes} min</span>
    </div>
    <div class="patient-meta">
      <span class="badge ${urgency.className}">${urgency.name}</span>
      <span class="score">Score: ${patient.priorityScore}</span>
      <span class="arrival">Arrival: ${new Date(patient.arrivalTime * 1000).toLocaleTimeString()}</span>
    </div>
  `;
  return article;
}

function renderQueues() {
  queuesContainer.innerHTML = "";

  clinics.forEach((clinic) => {
    const sortedPatients = getSortedPatientsForClinic(clinic.id);
    const card = document.createElement("section");
    card.className = "clinic-card";
    card.innerHTML = `
      <div class="clinic-header">
        <div>
          <h3>${clinic.name}</h3>
          <p>${clinic.specialty}</p>
        </div>
        <span class="queue-count">${sortedPatients.length} waiting</span>
      </div>
      <div class="queue-list"></div>
    `;

    const list = card.querySelector(".queue-list");
    if (sortedPatients.length === 0) {
      list.innerHTML = `<div class="empty-state">No patients waiting</div>`;
    } else {
      sortedPatients.forEach((patient, index) => {
        list.appendChild(renderPatient(patient, index + 1));
      });
    }

    queuesContainer.appendChild(card);
  });
}

clinicSelect.addEventListener("change", populateConditions);
conditionSelect.addEventListener("change", toggleOtherFields);
patientForm.addEventListener("submit", addPatient);
refreshBtn.addEventListener("click", renderQueues);

populateClinics();
populateConditions();
renderQueues();
setInterval(renderQueues, 5000);
