import { test, expect } from './fixture'

test('app signals ready after first render', async ({ page, goHome }) => {
  await goHome()
  // The sidebar is visible — React mounted and the app is ready.
})

test('create a list and add todos', async ({ page, goToTodos }) => {
  await goToTodos()

  // Wait for bridge to fully connect by checking console for the connection log
  await page.waitForFunction(() => {
    // The bridge connection logs to console on connect. But simpler: wait until
    // the TodosTab has finished its initial loadLists() by checking the DOM.
    // Either empty-state or todo-list will be present when the bridge is ready.
    return document.querySelector('[data-testid="empty-state"]') !== null
      || document.querySelector('[data-testid="todo-list"]') !== null
  }, { timeout: 10_000 })

  // Create a list with a unique name to avoid server state collision
  const listName = `Groceries-${Date.now()}`
  await page.getByTestId('new-list-input').fill(listName)
  await page.getByTestId('create-list-button').click()

  // Input clears — proves addList resolved on the bridge
  await expect(page.getByTestId('new-list-input')).toHaveValue('', { timeout: 5_000 })

  // List appears (signal-driven refresh from the C++ backend)
  const list = page.getByTestId('todo-list').filter({ hasText: listName })
  await expect(list).toBeVisible({ timeout: 10_000 })

  // Select the list
  await list.click()

  // Add items
  await page.getByTestId('new-item-input').fill('Milk')
  await page.getByTestId('add-item-button').click()
  await expect(page.getByText('Milk')).toBeVisible({ timeout: 5_000 })

  await page.getByTestId('new-item-input').fill('Eggs')
  await page.getByTestId('add-item-button').click()
  await expect(page.getByText('Eggs')).toBeVisible({ timeout: 5_000 })
})
