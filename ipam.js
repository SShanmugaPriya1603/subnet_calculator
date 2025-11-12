document.addEventListener('DOMContentLoaded', () => {
    const ipamForm = document.getElementById('ipam-form');
    const allocationList = document.getElementById('allocation-list');
    const subnetInput = document.getElementById('subnet-to-manage');

    // DOM elements for summary
    const totalHostsEl = document.getElementById('total-hosts');
    const allocatedHostsEl = document.getElementById('allocated-hosts');
    const remainingHostsEl = document.getElementById('remaining-hosts');

    // In-memory state to track host counts and allocations
    let state = {
        totalUsable: 0,
        allocations: [], // { dept: 'name', count: 120 }
    };

    // Calculate total usable hosts for a given CIDR
    function calculateUsableHosts(cidr) {
        if (cidr < 1 || cidr > 32) return 0;
        if (cidr > 30) return 0; // /31 and /32 have special rules, simplified here
        const hostBits = 32 - cidr;
        return Math.pow(2, hostBits) - 2;
    }

    // Update the entire UI based on the current state
    function updateUI() {
        // Calculate totals
        const totalAllocated = state.allocations.reduce((sum, alloc) => sum + alloc.count, 0);
        const remaining = state.totalUsable - totalAllocated;

        // Update summary display
        totalHostsEl.textContent = state.totalUsable.toLocaleString();
        allocatedHostsEl.textContent = totalAllocated.toLocaleString();
        remainingHostsEl.textContent = remaining.toLocaleString();

        // Update allocation list
        allocationList.innerHTML = ''; // Clear the list
        state.allocations.forEach(alloc => {
            const listItem = document.createElement('li');
            listItem.textContent = `[${alloc.dept}]: ${alloc.count.toLocaleString()} hosts`;
            allocationList.appendChild(listItem);
        });
    }

    // Initialize or reset the state when the subnet input changes
    function initializeState() {
        try {
            const cidr = parseInt(subnetInput.value.split('/')[1], 10);
            state.totalUsable = calculateUsableHosts(cidr);
            state.allocations = []; // Reset allocations
            updateUI();
        } catch (e) {
            alert('Invalid Subnet format. Please use x.x.x.x/y format.');
            state.totalUsable = 0;
            state.allocations = [];
            updateUI();
        }
    }

    // Initial setup
    initializeState();
    subnetInput.addEventListener('change', initializeState);

    ipamForm.addEventListener('submit', (event) => {
        event.preventDefault();

        const deptName = document.getElementById('dept-name').value;
        const hostsToAllocate = parseInt(document.getElementById('hosts-to-allocate').value, 10);

        if (!deptName || isNaN(hostsToAllocate) || hostsToAllocate <= 0) {
            alert('Please enter a valid department name and number of hosts.');
            return;
        }

        const totalAllocated = state.allocations.reduce((sum, alloc) => sum + alloc.count, 0);
        const remaining = state.totalUsable - totalAllocated;

        if (hostsToAllocate > remaining) {
            alert(`Allocation failed. Only ${remaining.toLocaleString()} hosts are available.`);
            return;
        }

        // Add the new allocation to our state
        state.allocations.push({
            dept: deptName,
            count: hostsToAllocate,
        });

        // Clear the form for the next entry
        document.getElementById('dept-name').value = '';
        document.getElementById('hosts-to-allocate').value = '';
        document.getElementById('dept-name').focus();

        // Re-render the entire UI with the new data
        updateUI();
    });
});