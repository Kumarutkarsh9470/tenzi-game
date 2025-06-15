import React, { useState, useEffect } from "react";
import Die from "./Die";

const GameBoard = ({ playerName, onPlayAgain }) => {
  const [dice, setDice] = useState(() => generateNewDice());
  const [tenzies, setTenzies] = useState(false);
  const [rollCount, setRollCount] = useState(0);

  function generateNewDice() {
    const newDice = [];
    for (let i = 0; i < 10; i++) {
      newDice.push({
        value: Math.ceil(Math.random() * 6),
        isHeld: false,
        id: i,
      });
    }
    return newDice;
  }

  useEffect(() => {
    const allHeld = dice.every((die) => die.isHeld);
    const firstValue = dice[0].value;
    const allSameValue = dice.every((die) => die.value === firstValue);
    if (allHeld && allSameValue) {
      setTenzies(true);
    }
  }, [dice]);

  const rollDice = () => {
    if (!tenzies) {
      setDice((oldDice) =>
        oldDice.map((die) => {
          return die.isHeld
            ? die
            : { ...die, value: Math.ceil(Math.random() * 6) };
        })
      );
      setRollCount((prev) => prev + 1);
    } else {
      setTenzies(false);
      setDice(generateNewDice());
      setRollCount(0);
    }
  };

  const holdDice = (id) => {
    setDice((oldDice) =>
      oldDice.map((die) => {
        return die.id === id ? { ...die, isHeld: !die.isHeld } : die;
      })
    );
  };

  const diceElements = dice.map((die) => (
    <Die
      key={die.id}
      value={die.value}
      isHeld={die.isHeld}
      onHold={() => holdDice(die.id)}
    />
  ));

  const playAgain = () => {
    setTenzies(false);
    setDice(generateNewDice());
    setRollCount(0);
    onPlayAgain();
  };

  return (
    <div className="game-board">
      <div className="game-header">
        <h2>Welcome, {playerName}!</h2>
        <p>
          Roll until all dice are the same. Click each die to freeze it at its
          current value between rolls.
        </p>
        <p>Rolls: {rollCount}</p>
      </div>

      {tenzies && (
        <div className="win-message">
          <h2>🎉 Congratulations {playerName}! 🎉</h2>
          <p>You won in {rollCount} rolls!</p>
        </div>
      )}

      <div className="dice-container">{diceElements}</div>

      <div className="button-container">
        <button className="roll-btn" onClick={rollDice}>
          {tenzies ? "New Game" : "Roll"}
        </button>

        <button className="play-again-btn" onClick={playAgain}>
          Play Again
        </button>
      </div>
    </div>
  );
};

export default GameBoard;
