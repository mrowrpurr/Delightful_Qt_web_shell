import type { Meta, StoryObj } from '@storybook/react-vite'
import { Select } from './select'

const meta = {
  title: 'UI/Select',
  component: Select,
} satisfies Meta<typeof Select>

export default meta
type Story = StoryObj<typeof meta>

export const Default: Story = {
  render: () => (
    <Select className="w-[200px]">
      <option value="">Choose a doc...</option>
      <option value="readme">README</option>
      <option value="architecture">Architecture</option>
      <option value="tutorial">Tutorial</option>
      <option value="testing">Testing</option>
    </Select>
  ),
}
