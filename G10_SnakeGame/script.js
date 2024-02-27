document.addEventListener("DOMContentLoaded", function() {
    fetch('G10_Homework_C10_SnakeGame/G10_Homework_C10_SnakeGame/scores.txt')
    .then(response => response.text())
    .then(data => {
        const scores = data.split('\n')
            .filter(score => score.trim() !== '') // Remove empty lines
            .map(score => parseInt(score.trim())) // Convert scores to integers
            .sort((a, b) => b - a); // Sort scores in descending order

        const scoreTable = document.getElementById("scoreTable");

        for (let i = 0; i < Math.min(scores.length, 5); i++) {
            const row = document.createElement("tr");
            row.innerHTML = `
                <td>${i + 1}</td>
                <td>${scores[i]}</td>
            `;
            scoreTable.appendChild(row);
        }
    })
    .catch(error => console.error("Error fetching scores:", error));
});
