document.addEventListener('DOMContentLoaded', () => {
    const subnetForm = document.getElementById('subnet-form');
    const resultsContainer = document.getElementById('results-container');
    const resultsTable = document.getElementById('results-table');
    const errorMessage = document.getElementById('error-message');

    subnetForm.addEventListener('submit', async (event) => {
        event.preventDefault();

        const ipAddress = document.getElementById('ip-address').value;
        const cidr = document.getElementById('cidr').value;

        // Show loading state
        resultsContainer.classList.remove('hidden');
        resultsTable.innerHTML = '<tr><td colspan="2">Calculating...</td></tr>';
        errorMessage.classList.add('hidden');
        resultsTable.classList.remove('hidden');

        try {
            // Send data to the C backend CGI script
            const response = await fetch('/cgi-bin/calculator.exe', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `ip=${encodeURIComponent(ipAddress)}&cidr=${encodeURIComponent(cidr)}`
            });

            if (!response.ok) {
                throw new Error(`Server responded with status: ${response.status}`);
            }

            const data = await response.json();

            if (data.error) {
                showError(data.error);
            } else {
                displayResults(data);
            }
        } catch (error) {
            console.error('Fetch Error:', error);
            showError('An unexpected error occurred. Please check the server configuration and network connection.');
        }
    });

    function displayResults(data) {
        resultsTable.innerHTML = `
            <tr><td>Network Address</td><td>${data.network_address}</td></tr>
            <tr><td>Subnet Mask</td><td>${data.subnet_mask}</td></tr>
            <tr><td>First Usable Host</td><td>${data.first_host}</td></tr>
            <tr><td>Last Usable Host</td><td>${data.last_host}</td></tr>
            <tr><td>Broadcast Address</td><td>${data.broadcast_address}</td></tr>
            <tr><td>Total Hosts</td><td>${data.total_hosts.toLocaleString()}</td></tr>
            <tr><td>Usable Hosts</td><td>${data.usable_hosts.toLocaleString()}</td></tr>
            <tr><td>IP Class</td><td>${data.ip_class}</td></tr>
            <tr><td>IP Type</td><td>${data.ip_type}</td></tr>
            <tr><td>IP in Binary</td><td class="binary-data">${data.ip_binary}</td></tr>
            <tr><td>Mask in Binary</td><td class="binary-data">${data.mask_binary}</td></tr>
        `;
        errorMessage.classList.add('hidden');
        resultsTable.classList.remove('hidden');
    }

    function showError(message) {
        errorMessage.textContent = message;
        errorMessage.classList.remove('hidden');
        resultsTable.classList.add('hidden');
    }
});
