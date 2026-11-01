(() => {
  "use strict";

  const CHOICES = ["rock", "paper", "scissors"];
  const LABELS = { rock: "Rock", paper: "Paper", scissors: "Scissors" };
  const BEATS = { rock: "scissors", scissors: "paper", paper: "rock" };
  const ANIMATION_DURATION = 1800;
  const REVEAL_DURATION = 600;

  const playerScoreEl = document.querySelector("#playerScore");
  const cpuScoreEl = document.querySelector("#cpuScore");
  const playerHandEl = document.querySelector("#playerHand");
  const cpuHandEl = document.querySelector("#cpuHand");
  const playerPlaceholder = document.querySelector("#playerPlaceholder");
  const cpuQuestion = document.querySelector("#cpuQuestion");
  const playerChoiceEl = document.querySelector("#playerChoice");
  const cpuChoiceEl = document.querySelector("#cpuChoice");
  const statusTitle = document.querySelector("#statusTitle");
  const statusDetail = document.querySelector("#statusDetail");
  const choiceButtons = [...document.querySelectorAll(".choice-button")];
  const resetButton = document.querySelector("#resetButton");

  let state = "IDLE";
  let playerScore = 0;
  let cpuScore = 0;
  let playerChoice = null;
  let cpuChoice = null;
  let animationTimer = null;
  let revealTimer = null;

  function setStatus(title, detail, outcome = "") {
    statusTitle.textContent = title;
    statusTitle.className = `status-title ${outcome}`.trim();
    statusDetail.textContent = detail;
  }

  function setButtonsDisabled(disabled) {
    choiceButtons.forEach(button => { button.disabled = disabled; });
  }

  function showHand(element, choice) {
    element.src = `assets/hands/${choice}.svg`;
    element.alt = `${LABELS[choice]} hand`;
    element.hidden = false;
  }

  function clearHand(element) {
    element.hidden = true;
    element.removeAttribute("src");
    element.alt = "";
  }

  function chooseCpuMove() {
    return CHOICES[Math.floor(Math.random() * CHOICES.length)];
  }

  function evaluate(player, cpu) {
    if (player === cpu) return "TIE";
    return BEATS[player] === cpu ? "WIN" : "LOSE";
  }

  function playRound(choice) {
    if (state === "ANIMATING") return;

    window.clearTimeout(animationTimer);
    window.clearTimeout(revealTimer);
    state = "ANIMATING";
    playerChoice = choice;
    cpuChoice = chooseCpuMove();

    playerPlaceholder.hidden = true;
    showHand(playerHandEl, playerChoice);
    clearHand(cpuHandEl);
    cpuQuestion.hidden = false;
    cpuQuestion.textContent = "?";
    playerChoiceEl.textContent = LABELS[playerChoice];
    cpuChoiceEl.textContent = "Hidden";
    setButtonsDisabled(true);
    setStatus("Throwing...", "Your opponent is making a move.");

    animationTimer = window.setTimeout(finishRound, ANIMATION_DURATION);
  }

  function finishRound() {
    const outcome = evaluate(playerChoice, cpuChoice);
    if (outcome === "WIN") playerScore += 1;
    if (outcome === "LOSE") cpuScore += 1;

    playerScoreEl.textContent = String(playerScore);
    cpuScoreEl.textContent = String(cpuScore);

    cpuQuestion.hidden = true;
    showHand(cpuHandEl, cpuChoice);
    cpuChoiceEl.textContent = LABELS[cpuChoice];
    state = "RESULT";

    if (outcome === "WIN") {
      setStatus("You Win!", `${LABELS[playerChoice]} beats ${LABELS[cpuChoice]}.`, "win");
    } else if (outcome === "LOSE") {
      setStatus("You Lose!", `${LABELS[cpuChoice]} beats ${LABELS[playerChoice]}.`, "lose");
    } else {
      setStatus("It's a Tie!", `You both chose ${LABELS[playerChoice]}.`, "tie");
    }

    // Keep the CPU reveal transition, then enable the next round.
    cpuHandEl.style.opacity = "0";
    requestAnimationFrame(() => {
      cpuHandEl.style.transition = `opacity ${REVEAL_DURATION}ms ease`;
      cpuHandEl.style.opacity = "1";
    });
    revealTimer = window.setTimeout(() => {
      setButtonsDisabled(false);
      state = "IDLE";
    }, REVEAL_DURATION);
  }

  function resetGame() {
    window.clearTimeout(animationTimer);
    window.clearTimeout(revealTimer);
    state = "IDLE";
    playerScore = 0;
    cpuScore = 0;
    playerChoice = null;
    cpuChoice = null;

    playerScoreEl.textContent = "0";
    cpuScoreEl.textContent = "0";
    clearHand(playerHandEl);
    clearHand(cpuHandEl);
    playerPlaceholder.hidden = false;
    cpuQuestion.hidden = false;
    cpuQuestion.textContent = "?";
    playerChoiceEl.textContent = "Waiting...";
    cpuChoiceEl.textContent = "Ready?";
    cpuHandEl.style.opacity = "";
    cpuHandEl.style.transition = "";
    setButtonsDisabled(false);
    setStatus("Choose your hand to begin!", "Rock beats Scissors · Scissors beats Paper · Paper beats Rock");
  }

  choiceButtons.forEach(button => {
    button.addEventListener("click", () => playRound(button.dataset.choice));
  });
  resetButton.addEventListener("click", resetGame);

  document.addEventListener("keydown", event => {
    if (event.key === "Escape") resetGame();
    if (state === "ANIMATING" || event.repeat) return;
    const shortcuts = { "1": "rock", "2": "paper", "3": "scissors" };
    if (shortcuts[event.key]) playRound(shortcuts[event.key]);
  });
})();
