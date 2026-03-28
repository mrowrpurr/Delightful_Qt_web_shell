import type { Preview } from '@storybook/react-vite'
import '../shared/styles/globals.css'

const preview: Preview = {
  parameters: {
    controls: {
      matchers: {
        color: /(background|color)$/i,
        date: /Date$/i,
      },
    },
    a11y: {
      test: 'todo',
    },
    backgrounds: {
      default: 'dark',
      values: [
        { name: 'dark', value: '#242424' },
        { name: 'light', value: '#ffffff' },
      ],
    },
  },
}

export default preview
